//
//  hfs_extent_ops.h
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_extents_h
#define hfsinspect_hfs_extents_h

#include "hfs/types.h"
#include "hfs/hfs_extentlist.h"

int hfs_get_extents_btree (BTreePtr *tree, const HFS *hfs) __attribute__((nonnull));
int hfs_extents_find_record (HFSPlusExtentRecord *record, hfs_block_t *record_start_block, const HFSFork *fork, size_t startBlock) __attribute__((nonnull));
int hfs_extents_compare_keys (const HFSPlusExtentKey *key1, const HFSPlusExtentKey *key2) __attribute__((nonnull));
bool hfs_extents_get_extentlist_for_fork (ExtentList* list, const HFSFork* fork) __attribute__((nonnull));
int hfs_extents_get_node (BTreeNodePtr *node, const BTreePtr bTree, bt_nodeid_t nodeNum) __attribute__((nonnull));

void swap_HFSPlusExtentKey          (HFSPlusExtentKey *record) __attribute__((nonnull));
void swap_HFSPlusExtentRecord       (HFSPlusExtentDescriptor record[]) __attribute__((nonnull));
void swap_HFSPlusExtentDescriptor   (HFSPlusExtentDescriptor *extent) __attribute__((nonnull));

#endif
