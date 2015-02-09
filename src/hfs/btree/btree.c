//
//  btree.c
//  hfsinspect
//
//  Created by Adam Knight on 11/26/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <stdio.h>
#include <errno.h>              // errno/perror
#include <string.h>             // memcpy, strXXX, etc.
#include <search.h>
#include <assert.h>             // assert()

#if defined(__linux__)
    #include <malloc.h>
    #define malloc_size malloc_usable_size
#else
    #include <malloc/malloc.h>      // malloc_size
#endif

#include "hfs/btree/btree.h"
#include "hfs/btree/btree_endian.h"
#include "hfsinspect/stringtools.h" // memdump
#include "volumes/utilities.h"      // fpread
#include "hfs/unicode.h"
#include "logging/logging.h"        // console printing routines


static inline int icmp(const void* a, const void* b)
{
    int64_t A = *(int64_t*)a;
    int64_t B = *(int64_t*)b;
    return cmp(A,B);
}

bool BTIsNodeUsed(const BTreePtr bTree, bt_nodeid_t nodeNum)
{
    if (bTree->_loadingBitmap == true)
        return true;

    assert(bTree);

    // Load the tree bitmap, if needed.
    if (bTree->nodeBitmap == NULL) {
        info("Loading tree bitmap.");
        bTree->_loadingBitmap = true;

        // Get the B-Tree's header node (#0)
        BTreeNodePtr node   = NULL;
        if ( BTGetNode(&node, bTree, 0) < 0) return false;

        // Get the third record (map data)
        BTNodeRecord record = {0};
        BTGetBTNodeRecord(&record, node, 2);

        // Copy the data out into a persistant buffer
        bTree->nodeBitmapSize = record.recordLen;

        // Initially allocate space for up to 16 nodes.
        ALLOC(bTree->nodeBitmap, bTree->headerRecord.nodeSize * 16);
        memcpy(bTree->nodeBitmap, record.record, record.recordLen);

        // Concat any linked map node records
        while (node->nodeDescriptor->fLink > 0 && (signed)node->nodeDescriptor->fLink != -1) {
            size_t      old_size = bTree->nodeBitmapSize;
            bt_nodeid_t node_id  = node->nodeDescriptor->fLink;
            info("Loading bitmap continuation node %d", node_id);

            BTFreeNode(node);
            node = NULL;

            BTGetNode(&node, bTree, node_id);
            assert(node != NULL);

            BTGetBTNodeRecord(&record, node, 0);
            bTree->nodeBitmapSize += record.recordLen;
            if (malloc_size(bTree->nodeBitmap) < bTree->nodeBitmapSize) {
                bTree->nodeBitmap = realloc(bTree->nodeBitmap, bTree->nodeBitmapSize);
                if (bTree->nodeBitmap == NULL) {
                    critical("Could not allocate memory for tree bitmap.");
                }
            }

            memcpy(bTree->nodeBitmap + old_size, record.record, record.recordLen);
        }
        info("Done loading nodes.");

        // Clean up
        BTFreeNode(node);

        bTree->_loadingBitmap = false;
    }

    // Check the bit in the data.
    bool result = BTIsBlockUsed(nodeNum, bTree->nodeBitmap, bTree->nodeBitmapSize);
    info("returning %d", result);

    return result;
}

bool BTIsBlockUsed(uint32_t thisAllocationBlock, void* allocationFileContents, size_t length)
{
    size_t idx      = (thisAllocationBlock / 8);
    if (idx > length) return false;

    Byte   thisByte = ((Bytes)allocationFileContents)[idx];
    return (thisByte & (1 << (7 - (thisAllocationBlock % 8)))) != 0;
}

int BTGetBTNodeRecord(BTNodeRecordPtr record, const BTreeNodePtr node, BTRecNum recNum)
{
    assert(record);
    assert(node);

    record->bTree  = node->bTree;
    record->node   = node;
    record->recNum = recNum;

    record->offset = BTGetRecordOffset(node, recNum);
    record->record = (node->data + record->offset);
    {
        BTRecOffset nextRecOff = 0;
        nextRecOff        = BTGetRecordOffset(node, recNum + 1);
        record->recordLen = (nextRecOff - record->offset);
    }

    record->key       = (BTreeKeyPtr)record->record;
    record->keyLen    = BTGetRecordKeyLength(node, recNum);

    record->value     = (record->record + record->keyLen);
    record->valueLen  = record->recordLen - record->keyLen;
    record->valueLen += (record->valueLen % 2);

    return 0;
}

BTRecOffset BTGetRecordOffset(const BTreeNodePtr node, uint16_t recNum)
{
    BTRecOffset    result      = 0;

    int            recordCount = node->nodeDescriptor->numRecords;
    BTRecOffsetPtr offsets     = ((BTRecOffsetPtr)(node->data + node->nodeSize)) - recordCount - 1;
    if ( offsets[recordCount] != 14 ) {
        memdump(stderr, node->data, node->nodeSize, 16, 4, 4, DUMP_FULL);
        critical("Bad sentinel @ %ld! (%d != 14)", (long)((char*)offsets - (char*)node->data) + recordCount, offsets[recordCount]); /*sizeof(BTNodeDescriptor)*/
    }
    result = offsets[recordCount - recNum];

    return result;
}

Bytes BTGetRecord(const BTreeNodePtr node, uint16_t recNum)
{
    return (node->data + BTGetRecordOffset(node, recNum));
}

uint16_t BTGetRecordKeyLength(const BTreeNodePtr node, uint16_t recNum)
{
    /*
       Immediately following the keyLength is the key itself. The length of the key is determined by the node type and the B-tree attributes. In leaf nodes, the length is always determined by keyLength. In index nodes, the length depends on the value of the kBTVariableIndexKeysMask bit in the B-tree attributes in the header record. If the bit is clear, the key occupies a constant number of bytes, determined by the maxKeyLength field of the B-tree header record. If the bit is set, the key length is determined by the keyLength field of the keyed record.

       Following the key is the record's data. The format of this data depends on the node type, as explained in the next two sections. However, the data is always aligned on a two-byte boundary and occupies an even number of bytes. To meet the first alignment requirement, a pad byte must be inserted between the key and the data if the size of the keyLength field plus the size of the key is odd. To meet the second alignment requirement, a pad byte must be added after the data if the data size is odd.

       http://developer.apple.com/legacy/library/technotes/tn/tn1150.html#KeyedRecords
     */

    BTHeaderRec       headerRecord = node->bTree->headerRecord;
    BTNodeDescriptor* nodeDesc     = (BTNodeDescriptor*)node->data;

    if ((nodeDesc->kind != kBTIndexNode) && (nodeDesc->kind != kBTLeafNode))
        return 0;

    Bytes       record  = BTGetRecord(node, recNum);
    BTRecOffset keySize = headerRecord.maxKeyLength;
    BTreeKey*   keyPtr  = (BTreeKey*)record;

    // Variable-length keys
    if (
        (nodeDesc->kind == kBTLeafNode) ||
        ((nodeDesc->kind == kBTIndexNode) && headerRecord.attributes & kBTVariableIndexKeysMask)
        ) {

        keySize = (
            (headerRecord.attributes & kBTBigKeysMask) ?
            (keyPtr->length16) :
            (keyPtr->length8)
            );
    }

    // Handle too-long keys
    // Note that while this will let us fight another day, it generally means Things Are Bad.
    if (keySize > headerRecord.maxKeyLength) {
        warning("Key length for (tree %u, node %u, record %u) was %u; the maximum for this B-tree is %u.", node->treeID, node->nodeNumber, recNum, keySize, headerRecord.maxKeyLength);
        keySize = headerRecord.maxKeyLength;
    }

    // Adjust for the initial key length field
    keySize += (
        (headerRecord.attributes & kBTBigKeysMask) ?
        sizeof(keyPtr->length16) :
        sizeof(keyPtr->length8)
        );

    // Pad to an even number of bytes
    keySize += keySize % 2;

    return keySize;
}

int btree_init(BTreePtr btree, FILE* fp)
{
    ssize_t nbytes;
    char    buf[512];

    // First, init the fork record for future calls.
    btree->fp = fp;

    // Now load up the header node.
    if ( (nbytes = fpread(btree->fp, &buf, sizeof(BTNodeDescriptor), 0)) < 0 )
        return -1;

    btree->nodeDescriptor = *(BTNodeDescriptor*)&buf;
    swap_BTNodeDescriptor(&btree->nodeDescriptor);

    // If there's a header record, we'll likely want that as well.
    if (btree->nodeDescriptor.numRecords > 0) {
        memset(buf, 0, 512);

        if ( (nbytes = fpread(btree->fp, &buf, sizeof(BTHeaderRec), sizeof(BTNodeDescriptor))) < 0 )
            return -1;

        btree->headerRecord = *(BTHeaderRec*)&buf;
        swap_BTHeaderRec(&btree->headerRecord);

        // Next comes 128 bytes of "user" space for record 2 that we don't care about.
        // TODO: Then the remainder of the node is allocation bitmap data for record 3 which we'll care about later.
    }

    // Init the node cache.
    cache_init(&btree->nodeCache, 100);

    return 0;
}

int btree_get_node(BTreeNodePtr* outNode, const BTreePtr tree, bt_nodeid_t nodeNumber)
{
    info("Getting node %d", nodeNumber);

    if(nodeNumber >= tree->headerRecord.totalNodes) {
        error("Node beyond file range.");
        return -1;
    }

    if ((nodeNumber != 0) && (BTIsNodeUsed(tree, nodeNumber) == false)) {
        info("Node %u is unused.", nodeNumber);
        return 0;
    }

    BTreeNodePtr node = NULL;
    ALLOC(node, sizeof(struct _BTreeNode));

    node->nodeSize   = tree->headerRecord.nodeSize;
    node->nodeNumber = nodeNumber;
    node->nodeOffset = node->nodeNumber * node->nodeSize;
    node->bTree      = tree;
    node->treeID     = tree->treeID;

    ALLOC(node->data, node->nodeSize);
    node->dataLen    = malloc_size(node->data);

    ssize_t result = 0;
    if (tree->nodeCache != NULL) {
        result = cache_get(tree->nodeCache, (void*)node->data, node->nodeSize, nodeNumber);
        if (result)
            info("Loaded a cached node for %u:%u", node->treeID, nodeNumber);
    }

    if (result < 1) {
        assert(node->data != NULL);
        result = fpread(node->bTree->fp, node->data, node->nodeSize, node->nodeOffset);
        if (result != node->nodeSize) {
            warning("node read failed: expected %zd bytes, got %zd", node->nodeSize, result);
        } else {
            if (result && (tree->nodeCache != NULL))
                result = cache_set(tree->nodeCache, (char*)node->data, node->nodeSize, nodeNumber);
        }
    }

    if (result < 0) {
        error("Error reading from fork.");
        btree_free_node(node);
        return result;
    }

    if ( swap_BTreeNode(node) < 0 ) {
        btree_free_node(node);
        warning("Byte-swap of node failed.");
        errno = EINVAL;
        return -1;
    }

    node->recordCount = node->nodeDescriptor->numRecords;

    *outNode          = node;

    return 0;
}

void btree_free_node (BTreeNodePtr node)
{
    if ((node != NULL) && (node->data != NULL)) {
        FREE(node->data);
        FREE(node);
    }
}

int btree_get_record(BTreeKeyPtr* key, Bytes* data, const BTreeNodePtr node, BTRecNum recordID)
{
    assert(key || data);
    assert(node);
    assert(node->bTree->headerRecord.nodeSize > 0);

    if (recordID >= node->recordCount) return -1;

    Bytes       record  = BTGetRecord(node, recordID);
    BTRecOffset keySize = BTGetRecordKeyLength(node, recordID);

    if (key != NULL)  *key = (BTreeKey*)record;
    if (data != NULL) *data = (record + keySize);

    return 0;
}

// Not a "real" tree walker; follows the linked list fields for the leaf nodes.
int btree_walk(const BTreePtr btree, const BTreeNodePtr node, btree_walk_func walker)
{
    BTreeNodePtr _node = node;

    // Fetch the first leaf node if no node was passed in.
    if (node == NULL) {
        BTGetNode(&_node, btree, btree->headerRecord.firstLeafNode);
    }

    walker(btree, _node);

    if ( (signed)_node->nodeDescriptor->fLink > 0 ) {
        BTreeNodePtr right = NULL;
        BTGetNode(&right, btree, _node->nodeDescriptor->fLink);
        btree_walk(btree, right, walker);
        btree_free_node(right);
    }

    if (!node && _node)
        btree_free_node(_node);

    return 0;
}

int btree_search(BTreeNodePtr* node, BTRecNum* recordID, const BTreePtr btree, const void* searchKey)
{
    debug("Searching tree %d for key of length %d", btree->treeID, ((BTreeKey*)searchKey)->length16);
    assert(node != NULL);
    assert(recordID != NULL);
    assert(btree != NULL);
    assert(searchKey != NULL);

    /*
       Starting at the node the btree defines as the root:
       Retrieve the node
       Look for the index in the node where the key should be (should return the exact record or the one that would be immediately before it)
       If the node is a leaf node then return whatever result we got back for the record index. Pass along the node search's result as ours.
       If the result is 0 (not found) and the index is non-zero, decrement it (WHY?)

       Fetch the record.
     */

    int          depth         = btree->headerRecord.treeDepth;
    bt_nodeid_t  currentNode   = btree->headerRecord.rootNode;
    BTreeNodePtr searchNode    = NULL;
    BTRecNum     searchIndex   = 0;
    bool         search_result = false;
    int          level         = depth;
    bt_nodeid_t  history[256]  = {0};

    if (level == 0) {
        error("Invalid tree (level 0?)");
        return false;
    }
    size_t num_nodes = btree->headerRecord.treeDepth;

    while (1) {
        if (currentNode > btree->headerRecord.totalNodes) {
            critical("Current node is %u but tree only has %u nodes. Aborting search.", currentNode, btree->headerRecord.totalNodes);
        }

        info("SEARCHING NODE %d (CNID %d; level %d)", currentNode, btree->treeID, level);

        errno = 0;
        void* r = lfind(&currentNode, &history, &num_nodes, sizeof(bt_nodeid_t), icmp);
        if (r == NULL) {
            if (errno) perror("lfind");
            history[level-1] = currentNode;
        } else {
            critical("Recursion detected in search!");
        }

        if ( BTGetNode(&searchNode, btree, currentNode) < 0) {
            error("search tree: failed to get node %u!", currentNode);
            perror("search");
            btree_free_node(searchNode);
            return false;
        }

        if (searchNode == NULL) {
            critical("Index node pointed to an empty node! (%u -> %u)", history[level+1], history[level]);
        }

        // Validate the node
        if (searchNode->nodeDescriptor->height != level) {
            // Nodes have a specific height.  This one fails.
            critical("Node found at unexpected height (got %d; expected %d).", searchNode->nodeDescriptor->height, level);
        }
        if (level == 1) {
            if (searchNode->nodeDescriptor->kind != kBTLeafNode) {
                critical("Node found at level 1 that is not a leaf node!");
            }
        } else {
            if (searchNode->nodeDescriptor->kind != kBTIndexNode) {
                critical("Invalid node! (expected an index node at level %d)", level);
            }
        }

        // Search the node
        search_result = btree_search_node(&searchIndex, btree, searchNode, searchKey);
        info("SEARCH NODE RESULT: %d; idx %d", search_result, searchIndex);

        // If this was a leaf node, return it as there's no next node to search.
        if ( ((BTNodeDescriptor*)searchNode->data)->kind == kBTLeafNode) {
            debug("Breaking on leaf node.");
            break;
        }

        BTNodeRecord currentRecord = {0};
        BTGetBTNodeRecord(&currentRecord, searchNode, searchIndex);

        BTreeKeyPtr  currentKey    = currentRecord.key;
        Bytes        currentValue  = currentRecord.value;

        info("FOLLOWING RECORD %d from node %d", searchIndex, searchNode->nodeNumber);

        currentNode = *( (bt_nodeid_t*)currentValue );
        if (currentNode == 0) {
            critical("Pointed to the header node.  That doesn't work...");
        }
        debug("Next node is %d", currentNode);

        if (btree->keyCompare(searchKey, currentKey) == -1) {
            critical("Search failed. Returned record's key is higher than the search key.");
        }

        btree_free_node(searchNode);
        searchNode = NULL;

        --level;

        continue;
    }

    // Either index is the node, or the insertion point where it would go.
    *node     = searchNode;
    *recordID = searchIndex;

    return search_result;
}

int btree_search_node(BTRecNum* index, const BTreePtr btree, const BTreeNodePtr node, const void* searchKey)
{
    assert(index != NULL);
    assert(node != NULL);
    assert(node->bTree->keyCompare != NULL);
    assert(searchKey != NULL);

    // Perform a binary search within the records (since they are guaranteed to be ordered).
    int low                        = 0;
    int high                       = node->nodeDescriptor->numRecords - 1;
    int result                     = 0;
    int recNum                     = 0;

    /*
       Low is 0, high is the index of the largest record.
       recNum is the record we're searching right now.

       Using a >>1 shift to divide to avoid DIV/0 errors when recNum is 0

       Key compare based on btree type.
       -1/0/1 results for k1 less/equal/greater than k2 (if k1 is gt k2, return 1, etc.)

       if result is that k1 is less than k2, remove the upper half of the search area (my key is less than the current key)
       if k1 is greater than k2, remove the lower half of the search area (my key is greater than the current key)
       if they're equal, return the index.

       if low is ever incremented over high, or high decremented over low, return the current index.
     */

    BTreeKeyPtr            testKey = NULL;
    btree_key_compare_func keyFunc = node->bTree->keyCompare;

    bool                   found   = false;

    //FIXME: When low==high but the recNum is != either, it will find the record prior to the correct one (actually, it returns the right one but tree search decrements it).
    while (low <= high) {
        recNum = (low + high) >> 1;

        btree_get_record(&testKey, NULL, node, recNum);
        result = keyFunc(searchKey, testKey);

        if (result < 0) {
            high = recNum - 1;
        } else if (result > 0) {
            low = recNum + 1;
        } else {
            found = 1;
            break;
        }
    }

    // Back up one if the last test key was greater (we need to always return the insertion point if we don't have a match).
    if (!found && (recNum != 0) && (result < 0)) recNum--;

    *index = recNum;
    return found;
}

