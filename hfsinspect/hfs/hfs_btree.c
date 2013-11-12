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

hfs_fork_type HFSDataForkType = 0x00;
hfs_fork_type HFSResourceForkType = 0xFF;

#pragma mark Tree Abstractions

ssize_t hfs_btree_init(HFSBTree *tree, const HFSFork *fork)
{
    debug("Creating B-Tree for CNID %d", fork->cnid);
    
	ssize_t size1 = 0, size2 = 0;
    
    // First, init the fork record for future calls.
    tree->fork = *fork;
    
    // Now load up the header node.
    char* buf;
    INIT_BUFFER(buf, 512);
    
    size1 = hfs_read_fork_range(buf, &tree->fork, sizeof(BTNodeDescriptor), 0);
    
    tree->nodeDescriptor = *( (BTNodeDescriptor*) buf );
    
    if (size1)
        swap_BTNodeDescriptor(&tree->nodeDescriptor);
    
    // If there's a header record, we'll likely want that as well.
    if (tree->nodeDescriptor.numRecords > 0) {
        memset(buf, 0, malloc_size(buf));
        
        size2 = hfs_read_fork_range(buf, fork, sizeof(BTHeaderRec), sizeof(BTNodeDescriptor));
        tree->headerRecord = *( (BTHeaderRec*) buf);
        
        if (size2)
            swap_BTHeaderRec(&tree->headerRecord);
        
        // Then 128 bytes of "user" space for record 2.
        // Then the remainder of the node is allocation bitmap data for record 3.
    }
    
    FREE_BUFFER(buf);
    
    return (size1 + size2);
}

int8_t hfs_btree_get_node (HFSBTreeNode *out_node, const HFSBTree *tree, hfs_node_id nodeNumber)
{
    info("Getting node %d", nodeNumber);
    
    if(nodeNumber > tree->headerRecord.totalNodes) {
        error("Node beyond file range.");
        return -1;
    }
    
    HFSBTreeNode node;
    node.nodeSize       = tree->headerRecord.nodeSize;
    node.nodeNumber     = nodeNumber;
    node.nodeOffset     = node.nodeNumber * node.nodeSize;;
    node.bTree          = *tree;
    
    debug("Reading %zu bytes at logical offset %u", node.nodeSize, node.nodeOffset);
    
    ssize_t result = 0;
    char* data;
    INIT_BUFFER(data, node.nodeSize);
    
    result = hfs_read_fork_range(data, &node.bTree.fork, node.nodeSize, node.nodeOffset);
    
    if (result < 0) {
        error("Error reading from fork.");
        FREE_BUFFER(data);
        return result;
        
    } else {
        node.buffer = buffer_alloc(node.nodeSize);
        buffer_set(&node.buffer, data, malloc_size(data));
        FREE_BUFFER(data);
        
        int swap_result = swap_BTreeNode(&node);
        if (swap_result == -1) {
            buffer_free(&node.buffer);
            debug("Byte-swap of node failed.");
            errno = EINVAL;
            return -1;
        }
    }
    
    node.recordCount = node.nodeDescriptor.numRecords;
    
    *out_node = node;
    return 1;
}

void hfs_btree_free_node (HFSBTreeNode *node)
{
    debug("Freeing node %d", node->nodeNumber);
    
    buffer_free(&node->buffer);
}

int icmp(const void* a, const void* b)
{
    if (a == b) {
        return 0;
    } else return a > b;
}

bool hfs_btree_search_tree(HFSBTreeNode *node, hfs_record_id *index, const HFSBTree *tree, const void *searchKey)
{
    debug("Searching tree %d for key of length %d", tree->fork.cnid, ((BTreeKey*)searchKey)->length16 );
    
    /*
     Starting at the node the btree defines as the root:
     Retrieve the node
     Look for the index in the node where the key should be (should return the exact record or the one that would be immediately before it)
     If the node is a leaf node then return whatever result we got back for the record index. Pass along the node search's result as ours.
     If the result is 0 (not found) and the index is non-zero, decrement it (WHY?)
     
     Fetch the record.
     */
    
    int depth                   = tree->headerRecord.treeDepth;
    hfs_node_id currentNode     = tree->headerRecord.rootNode;
    HFSBTreeNode searchNode;
    hfs_record_id searchIndex   = 0;
    bool search_result          = false;
    int level                   = depth;
    
    if (level == 0) {
        error("Invalid tree (level 0?)");
        PrintBTHeaderRecord(&tree->headerRecord);
        return false;
    }

    hfs_node_id history[depth];
    for (int i = 0; i < depth; i++) history[i] = 0;
    
    while (1) {
        if (currentNode > tree->headerRecord.totalNodes) {
            critical("Current node is %u but tree only has %u nodes. Aborting search.", currentNode, tree->headerRecord.totalNodes);
        }
        
        info("SEARCHING NODE %d (CNID %d; level %d)", currentNode, tree->fork.cnid, level);
        
        errno = 0;
        size_t num_nodes = tree->headerRecord.treeDepth;
        void* r = lfind(&currentNode, &history, &num_nodes, sizeof(hfs_node_id), icmp);
        if (r == NULL) {
            if (errno) perror("search");
            history[level-1] = currentNode;
        } else {
            critical("Recursion detected in search!");
        }
        
        int node_result = hfs_btree_get_node(&searchNode, tree, currentNode);
        if (node_result == -1) {
            error("search tree: failed to get node %u!", currentNode);
            perror("search");
            return false;
        }
        
        // Validate the node
        if (searchNode.nodeDescriptor.height != level) {
            // Nodes have a specific height.  This one fails.
            critical("Node found at unexpected height (got %d; expected %d).", searchNode.nodeDescriptor.height, level);
        }
        if (level == 1) {
            if (searchNode.nodeDescriptor.kind != kBTLeafNode) {
                critical("Node found at level 1 that is not a leaf node!");
            }
        } else {
            if (searchNode.nodeDescriptor.kind != kBTIndexNode) {
                PrintBTNodeDescriptor(&searchNode.nodeDescriptor);
                critical("Invalid node! (expected an index node at level %d)", level);
            }
        }
        
        // Search the node
        search_result = hfs_btree_search_node(&searchIndex, &searchNode, searchKey);
        info("SEARCH NODE RESULT: %d; idx %d", search_result, searchIndex);
        
//        if (getenv("DEBUG")) {
//            printf("History: "); for (int i = 0; i < depth; i++) { printf("%d:", history[i]); } printf("\n");
//        }

        // If this was a leaf node, return it as there's no next node to search.
        int8_t nodeKind = ((BTNodeDescriptor*)searchNode.buffer.data)->kind;
        if (nodeKind == kBTLeafNode) {
            debug("Breaking on leaf node.");
            break;
        }
        
        HFSBTreeNodeRecord *record = &searchNode.records[searchIndex];
        info("FOLLOWING RECORD %d from node %d", searchIndex, searchNode.nodeNumber);
        
//        if (getenv("DEBUG")) {
//            if (tree->fork.cnid == kHFSCatalogFileID)
//                VisualizeHFSPlusCatalogKey(record->key, "Returned Key", 1);
//            else if (tree->fork.cnid == kHFSExtentsFileID)
//                VisualizeHFSPlusExtentKey(record->key, "Returned Key", 1);
//        }
        
        currentNode = *( (hfs_node_id*)record->value );
        if (currentNode == 0) {
            debug("Record size: %zd", record->length);
            VisualizeData(record->record, record->length);
            critical("Pointed to the header node.  That doesn't work...");
        }
        debug("Next node is %d", currentNode);
        
        if (tree->keyCompare(searchKey, record->key) == -1) {
            critical("Search failed. Returned record's key is higher than the search key.");
        }
        
        hfs_btree_free_node(&searchNode);
        
        --level;
        
        continue;
    }

    // Either index is the node, or the insertion point where it would go.
    *node = searchNode; //Return a brand new node.
    *index = searchIndex;
    return search_result;
}

// Returns 1/0 for found and the index where the record is/would be.
bool hfs_btree_search_node(hfs_record_id *index, const HFSBTreeNode *node, const void *searchKey)
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
        critical("Unsupported B-Tree: %u", node->bTree.fork.cnid);
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

unsigned hfs_btree_iterate_tree_leaves (void* context, HFSBTree *tree, bool(*process)(void* context, HFSBTreeNode* node))
{
    return 0;
}

unsigned hfs_btree_iterate_node_records (void* context, HFSBTreeNode* node, bool(*process)(void* context, HFSBTreeNodeRecord* record))
{
    hfs_node_id processed = 0;
    for (unsigned recNum = 0; recNum < node->nodeDescriptor.numRecords; recNum++) {
        if (! process(context, &node->records[recNum]) ) break;
        processed++;
    }
    return processed;
}

// Wait until we process the node allocation bitmap...
unsigned hfs_btree_iterate_all_nodes (void* context, HFSBTree *tree, bool(*process)(void* context, HFSBTreeNode* node))
{
    return 0;
}

unsigned hfs_btree_iterate_all_records (void* context, HFSBTree *tree, bool(*process)(void* context, HFSBTreeNodeRecord* record))
{
    hfs_node_id cnid = tree->headerRecord.firstLeafNode;
    hfs_node_id processed = 0;
    
    while (1) {
        HFSBTreeNode node;
        int8_t result = hfs_btree_get_node(&node, tree, cnid);
        if (result < 0) {
            perror("get node");
            critical("There was an error fetching node %d", cnid);
        }
        
        // Process node
        for (unsigned recNum = 0; recNum < node.nodeDescriptor.numRecords; recNum++) {
            if (! process(context, &node.records[recNum]) ) return processed;
            processed++;
        }
    }
}
