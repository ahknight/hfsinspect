//
//  hfs_btree.c
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs_btree.h"
#include "output.h"
#include "hfs_io.h"
#include "hfs_endian.h"
#include "output_hfs.h"
#include <search.h>
#include <stdlib.h>

hfs_forktype_t HFSDataForkType = 0x00;
hfs_forktype_t HFSResourceForkType = 0xFF;

#pragma mark Tree Abstractions

int btree_init(BTree *tree, const HFSFork *fork)
{
    debug("Creating B-Tree for CNID %d", fork->cnid);
    
    ssize_t nbytes;
    char buf[512];
    
    // First, init the fork record for future calls.
//    tree->fork = *fork;
    tree->treeID = fork->cnid;
    tree->fp = fopen_hfsfork((HFSFork*)fork);
    
    // Now load up the header node.
    if ( (nbytes = fpread(tree->fp, &buf, sizeof(BTNodeDescriptor), 0)) < 0 )
        return -1;
    
    tree->nodeDescriptor = *(BTNodeDescriptor*)&buf;
    swap_BTNodeDescriptor(&tree->nodeDescriptor);
    
    // If there's a header record, we'll likely want that as well.
    if (tree->nodeDescriptor.numRecords > 0) {
        memset(buf, 0, 512);
        
        if ( (nbytes = fpread(tree->fp, &buf, sizeof(BTHeaderRec), sizeof(BTNodeDescriptor))) < 0 )
            return -1;
        
        tree->headerRecord = *(BTHeaderRec*)&buf;
        swap_BTHeaderRec(&tree->headerRecord);
        
        // Next comes 128 bytes of "user" space for record 2 that we don't care about.
        // TODO: Then the remainder of the node is allocation bitmap data for record 3 which we'll care about later.
    }
    
    // Init the node cache.
    cache_init(&tree->nodeCache, 100);
    
    return 0;
}

int btree_get_node (BTreeNode** out_node, const BTree *tree, bt_nodeid_t nodeNumber)
{
    info("Getting node %d", nodeNumber);
    
    if(nodeNumber >= tree->headerRecord.totalNodes) {
        error("Node beyond file range.");
        return -1;
    }
    
    BTreeNode* node;
    INIT_BUFFER(node, sizeof(BTreeNode));
    
    node->nodeSize      = tree->headerRecord.nodeSize;
    node->nodeNumber    = nodeNumber;
    node->nodeOffset    = node->nodeNumber * node->nodeSize;;
    node->bTree         = *tree;
    node->treeID        = tree->treeID;
    
    ssize_t result = 0;
    char* data;
    INIT_BUFFER(data, node->nodeSize);
    
    if (tree->nodeCache != NULL) {
        result = cache_get(tree->nodeCache, (void*)data, node->nodeSize, nodeNumber);
        if (result)
            info("Loaded a cached node for %u:%u", tree->treeID, nodeNumber);
    }
    
    if (result < 1) {
        result = fpread(node->bTree.fp, data, node->nodeSize, node->nodeOffset);
        if (result && tree->nodeCache != NULL)
            result = cache_set(tree->nodeCache, data, node->nodeSize, nodeNumber);
    }
    
    if (result < 0) {
        error("Error reading from fork.");
        FREE_BUFFER(data);
        FREE_BUFFER(node);
        return result;
        
    } else {
        node->dataLen = malloc_size(data);
        INIT_BUFFER(node->data, node->dataLen);
        memcpy(node->data, data, node->dataLen);
        FREE_BUFFER(data);
        
        if ( swap_BTreeNode(node) < 0 ) {
            FREE_BUFFER(node->data);
            FREE_BUFFER(node);
            warning("Byte-swap of node failed.");
            errno = EINVAL;
            return -1;
        }
    }
    
    node->recordCount = node->nodeDescriptor.numRecords;
    
    *out_node = node;
    
    return 0;
}

void btree_free_node (BTreeNode *node)
{
    if (node != NULL && node->data != NULL) {
        FREE_BUFFER(node->data);
        FREE_BUFFER(node);
    }
}

int icmp(const void* a, const void* b)
{
    return (a > b ? 1 : (a < b ? -1 : 0));
//    if (a == b) {
//        return 0;
//    } else return a > b;
}

bool btree_search_tree(BTreeNode** node, bt_recordid_t *index, const BTree *tree, const void *searchKey)
{
    debug("Searching tree %d for key of length %d", tree->treeID, ((BTreeKey*)searchKey)->length16 );
//    if (DEBUG)
//        if (tree->treeID == kHFSExtentsFileID)
//            VisualizeHFSPlusExtentKey(searchKey, "Search Key", 1);
    
    /*
     Starting at the node the btree defines as the root:
     Retrieve the node
     Look for the index in the node where the key should be (should return the exact record or the one that would be immediately before it)
     If the node is a leaf node then return whatever result we got back for the record index. Pass along the node search's result as ours.
     If the result is 0 (not found) and the index is non-zero, decrement it (WHY?)
     
     Fetch the record.
     */
    
    int depth                   = tree->headerRecord.treeDepth;
    bt_nodeid_t currentNode     = tree->headerRecord.rootNode;
    BTreeNode* searchNode;
    bt_recordid_t searchIndex   = 0;
    bool search_result          = false;
    int level                   = depth;
    
    if (level == 0) {
        error("Invalid tree (level 0?)");
        PrintBTHeaderRecord(&tree->headerRecord);
        return false;
    }

    bt_nodeid_t history[depth];
    for (int i = 0; i < depth; i++) history[i] = 0;
    
    while (1) {
        if (currentNode > tree->headerRecord.totalNodes) {
            critical("Current node is %u but tree only has %u nodes. Aborting search.", currentNode, tree->headerRecord.totalNodes);
        }
        
        info("SEARCHING NODE %d (CNID %d; level %d)", currentNode, tree->treeID, level);
        
        errno = 0;
        size_t num_nodes = tree->headerRecord.treeDepth;
        void* r = lfind(&currentNode, &history, &num_nodes, sizeof(bt_nodeid_t), icmp);
        if (r == NULL) {
            if (errno) perror("search");
            history[level-1] = currentNode;
        } else {
            critical("Recursion detected in search!");
        }
        
        if ( btree_get_node(&searchNode, tree, currentNode) < 0) {
            error("search tree: failed to get node %u!", currentNode);
            perror("search");
            return false;
        }
        
        // Validate the node
        if (searchNode->nodeDescriptor.height != level) {
            // Nodes have a specific height.  This one fails.
            critical("Node found at unexpected height (got %d; expected %d).", searchNode->nodeDescriptor.height, level);
        }
        if (level == 1) {
            if (searchNode->nodeDescriptor.kind != kBTLeafNode) {
                critical("Node found at level 1 that is not a leaf node!");
            }
        } else {
            if (searchNode->nodeDescriptor.kind != kBTIndexNode) {
                PrintBTNodeDescriptor(&searchNode->nodeDescriptor);
                critical("Invalid node! (expected an index node at level %d)", level);
            }
        }
        
        // Search the node
        search_result = btree_search_node(&searchIndex, searchNode, searchKey);
        info("SEARCH NODE RESULT: %d; idx %d", search_result, searchIndex);
        
//        if (DEBUG) {
//            printf("History: "); for (int i = 0; i < depth; i++) { printf("%d:", history[i]); } printf("\n");
//        }

        // If this was a leaf node, return it as there's no next node to search.
        if ( ((BTNodeDescriptor*)searchNode->data)->kind == kBTLeafNode) {
            debug("Breaking on leaf node.");
            break;
        }
        
        BTreeRecord *record = &searchNode->records[searchIndex];
        info("FOLLOWING RECORD %d from node %d", searchIndex, searchNode->nodeNumber);
        
//        if (DEBUG) {
//            if (tree->treeID == kHFSCatalogFileID)
//                VisualizeHFSPlusCatalogKey((void*)record->key, "Returned Key", 1);
//            else if (tree->treeID == kHFSExtentsFileID)
//                VisualizeHFSPlusExtentKey((void*)record->key, "Returned Key", 1);
//        }
        
        currentNode = *( (bt_nodeid_t*)record->value );
        if (currentNode == 0) {
            debug("Record size: %zd", record->length);
            VisualizeData(record->record, record->length);
            critical("Pointed to the header node.  That doesn't work...");
        }
        debug("Next node is %d", currentNode);
        
        if (tree->keyCompare(searchKey, record->key) == -1) {
            critical("Search failed. Returned record's key is higher than the search key.");
        }
        
        btree_free_node(searchNode);
        
        --level;
        
        continue;
    }

    // Either index is the node, or the insertion point where it would go.
    *node = searchNode;
    *index = searchIndex;
    
    return search_result;
}

// Returns 1/0 for found and the index where the record is/would be.
bool btree_search_node(bt_recordid_t *index, const BTreeNode *node, const void *searchKey)
{
//    debug("Search node %d for key of length %d", node->nodeNumber, ((BTreeKey*)searchKey)->length16);
    
    // Perform a binary search within the records (since they are guaranteed to be ordered).
    int low = 0;
    int high = node->nodeDescriptor.numRecords - 1;
    int result = 0;
    int recNum = 0;
    
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
    
    hfs_compare_keys keyFunc = node->bTree.keyCompare;
    void* testKey = NULL;
    
    if (keyFunc == NULL) {
        critical("Unsupported B-Tree: %u", node->bTree.treeID);
        return false;
    }
    
    bool found = false;
    
    //FIXME: When low==high but the recNum is != either, it will find the record prior to the correct one (actually, it returns the right one but tree search decrements it).
    while (low <= high) {
        recNum = (low + high) / 2;
        
        testKey = node->records[recNum].key;
        result = keyFunc(searchKey, testKey);
//        debug("record %d: key compare result: %d", recNum, result);
        
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
    if (!found && recNum != 0 && result < 0) recNum--;
    
    *index = recNum;
    return found;
}

unsigned btree_iterate_tree_leaves (void* context, BTree *tree, bool(*process)(void* context, BTreeNode* node))
{
    return 0;
}

unsigned btree_iterate_node_records (void* context, BTreeNode* node, bool(*process)(void* context, BTreeRecord* record))
{
    bt_nodeid_t processed = 0;
    for (unsigned recNum = 0; recNum < node->nodeDescriptor.numRecords; recNum++) {
        if (! process(context, &node->records[recNum]) ) break;
        processed++;
    }
    return processed;
}

// Wait until we process the node allocation bitmap...
unsigned btree_iterate_all_nodes (void* context, BTree *tree, bool(*process)(void* context, BTreeNode* node))
{
    return 0;
}

unsigned btree_iterate_all_records (void* context, BTree *tree, bool(*process)(void* context, BTreeRecord* record))
{
    bt_nodeid_t cnid = tree->headerRecord.firstLeafNode;
    bt_nodeid_t processed = 0;
    
    while (1) {
        BTreeNode* node = NULL;
        if ( btree_get_node(&node, tree, cnid) < 0) {
            perror("get node");
            critical("There was an error fetching node %d", cnid);
            return -1;
        }
        
        // Process node
        for (unsigned recNum = 0; recNum < node->nodeDescriptor.numRecords; recNum++) {
            if (! process(context, &node->records[recNum]) ) return processed;
            processed++;
        }
    }
}
