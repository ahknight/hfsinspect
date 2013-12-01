//
//  hfs_btree.c
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs/hfs_btree.h"
#include "misc/output.h"
#include "hfs/hfs_io.h"
#include "hfs/hfs_endian.h"
#include "hfs/output_hfs.h"
#include <search.h>
#include <stdlib.h>

/*
 The B-tree files are an array of blocks addressed by their order in the file.  Each block has a header with information on how it is positioned in the tree as well as prev/next pointers for its position in a doubly-linked list of that type of node. Node 0 holds the metadata for the tree including the ID of the root node and the node size.
 */


#pragma mark Tree Abstractions

int hfs_get_btree(BTreePtr *btree, const HFS *hfs, hfs_cnid_t cnid)
{
    //    static BTreePtr trees[16] = {{0}};
    BTreePtr tree = NULL;
    HFSFork *fork = NULL;
    
    INIT_BUFFER(tree, sizeof(struct _BTree));
    
    if ( hfsfork_get_special(&fork, hfs, cnid) < 0 ) {
        critical("Could not create fork for Attributes BTreePtr!");
        return -1;
    }
    FILE* fp = fopen_hfsfork(fork);
    btree_init(tree, fp);
    tree->treeID = cnid;
    switch (cnid) {
        case kHFSAttributesFileID:
//            tree->keyCompare = (btree_key_compare_func)hfs_attributes_compare_keys;
            break;
            
        default:
            break;
    }
//    tree->keyCompare = (btree_key_compare_func)hfs_attributes_compare_keys;
    
    
    return 0;
}

//unsigned btree_iterate_tree_leaves (void* context, BTreePtr tree, bool(*process)(void* context, BTreeNodePtr node))
//{
//    return 0;
//}
//
//unsigned btree_iterate_node_records (void* context, BTreeNodePtr node, bool(*process)(void* context, BTreeRecord* record))
//{
//    bt_nodeid_t processed = 0;
//    for (unsigned recNum = 0; recNum < node->nodeDescriptor.numRecords; recNum++) {
//        if (! process(context, &node->records[recNum]) ) break;
//        processed++;
//    }
//    return processed;
//}
//
//// Wait until we process the node allocation bitmap...
//unsigned btree_iterate_all_nodes (void* context, BTreePtr tree, bool(*process)(void* context, BTreeNodePtr node))
//{
//    return 0;
//}
//
//unsigned btree_iterate_all_records (void* context, BTreePtr tree, bool(*process)(void* context, BTreeRecord* record))
//{
//    bt_nodeid_t cnid = tree->headerRecord.firstLeafNode;
//    bt_nodeid_t processed = 0;
//    
//    while (1) {
//        BTreeNodePtr node = NULL;
//        if ( btree_get_node(&node, tree, cnid) < 0) {
//            perror("get node");
//            critical("There was an error fetching node %d", cnid);
//            return -1;
//        }
//        
//        // Process node
//        for (unsigned recNum = 0; recNum < node->nodeDescriptor.numRecords; recNum++) {
//            if (! process(context, &node->records[recNum]) ) return processed;
//            processed++;
//        }
//    }
//}
