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

extern hfs_fork_type HFSDataForkType;
extern hfs_fork_type HFSResourceForkType;

ssize_t hfs_btree_init          (HFSBTree *tree, const HFSFork *fork);

int8_t  hfs_btree_get_node      (HFSBTreeNode *node, const HFSBTree *tree, hfs_node_id nodeNumber);
void    hfs_btree_free_node     (HFSBTreeNode *node);

bool    hfs_btree_search_tree   (HFSBTreeNode *node, hfs_record_id *index, const HFSBTree *tree, const void *searchKey);
bool    hfs_btree_search_node   (hfs_record_id *index, const HFSBTreeNode *node, const void *searchKey);


// Linear processing of a B-Tree.  Useful for statistics and other whole-tree operations.
// Each returns the number of processed things. Return false on your processor to halt and return immediately.
unsigned    hfs_btree_iterate_tree_leaves       (void* context, HFSBTree *tree,     bool(*)(void* context, HFSBTreeNode* node));
unsigned    hfs_btree_iterate_node_records      (void* context, HFSBTreeNode* node, bool(*)(void* context, HFSBTreeNodeRecord* record));

unsigned    hfs_btree_iterate_all_nodes         (void* context, HFSBTree *tree,     bool(*)(void* context, HFSBTreeNode* node));
unsigned    hfs_btree_iterate_all_records       (void* context, HFSBTree *tree,     bool(*)(void* context, HFSBTreeNodeRecord* record));

#endif
