//
//  hfs_btree.h
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_btree_h
#define hfsinspect_hfs_btree_h

#include "hfs_structs.h"

enum {
    kBTHFSTreeType = 0,
    kBTUserTreeType = 128,
    kBTReservedTreeType = 255,
};

extern hfs_forktype_t HFSDataForkType;
extern hfs_forktype_t HFSResourceForkType;

static inline int cmp(int64_t a, int64_t b) {
    return ( ((a) > (b)) ? 1 : ( ((a) < (b)) ? -1 : 0 ) );
}

int     btree_init          (BTree *tree, const HFSFork *fork);

int     btree_get_node      (BTreeNode** node, const BTree *tree, bt_nodeid_t nodeNumber);
void    btree_free_node     (BTreeNode* node);

bool    btree_search_tree   (BTreeNode** node, bt_recordid_t *index, const BTree *tree, const void *searchKey);
bool    btree_search_node   (bt_recordid_t *index, const BTreeNode *node, const void *searchKey);


// Linear processing of a B-Tree.  Useful for statistics and other whole-tree operations.
// Each returns the number of processed things. Return false on your processor to halt and return immediately.
unsigned    btree_iterate_tree_leaves   (void* context, BTree *tree,     bool(*)(void* context, BTreeNode* node));
unsigned    btree_iterate_node_records  (void* context, BTreeNode* node, bool(*)(void* context, BTreeRecord* record));

unsigned    btree_iterate_all_nodes     (void* context, BTree *tree,     bool(*)(void* context, BTreeNode* node));
unsigned    btree_iterate_all_records   (void* context, BTree *tree,     bool(*)(void* context, BTreeRecord* record));

#endif
