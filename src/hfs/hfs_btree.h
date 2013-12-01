//
//  hfs_btree.h
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_btree_h
#define hfsinspect_hfs_btree_h

#include "hfs/hfs_structs.h"
#include "btree/btree.h"

enum {
    kBTHFSTreeType = 0,
    kBTUserTreeType = 128,
    kBTReservedTreeType = 255,
};

int hfs_get_btree(BTreePtr *btree, const HFS *hfs, hfs_cnid_t cnid);

// Linear processing of a B-Tree.  Useful for statistics and other whole-tree operations.
// Each returns the number of processed things. Return false on your processor to halt and return immediately.
//unsigned    btree_iterate_tree_leaves   (void* context, BTreePtr *tree,     bool(*)(void* context, BTreeNodePtr node));
//unsigned    btree_iterate_node_records  (void* context, BTreeNodePtr node, bool(*)(void* context, BTreeRecord* record));
//
//unsigned    btree_iterate_all_nodes     (void* context, BTreePtr *tree,     bool(*)(void* context, BTreeNodePtr node));
//unsigned    btree_iterate_all_records   (void* context, BTreePtr *tree,     bool(*)(void* context, BTreeRecord* record));

#endif
