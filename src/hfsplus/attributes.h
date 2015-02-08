//
//  attributes.h
//  hfsinspect
//
//  Created by Adam Knight on 6/16/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_attribute_ops_h
#define hfsinspect_hfs_attribute_ops_h

#include "hfsinspect/types.h"
#include "hfs/types.h"

int hfs_get_attribute_btree     (BTreePtr* tree, const HFS* hfs) __attribute__((nonnull));
int hfs_attributes_compare_keys (const HFSPlusAttrKey* key1, const HFSPlusAttrKey* key2) __attribute__((nonnull));
int hfs_attributes_get_node     (BTreeNodePtr* node, const BTreePtr bTree, bt_nodeid_t nodeNum) __attribute__((nonnull));

void swap_HFSPlusAttrKey        (HFSPlusAttrKey* record) __attribute__((nonnull));
void swap_HFSPlusAttrData       (HFSPlusAttrData* record) __attribute__((nonnull));
void swap_HFSPlusAttrForkData   (HFSPlusAttrForkData* record) __attribute__((nonnull));
void swap_HFSPlusAttrExtents    (HFSPlusAttrExtents* record) __attribute__((nonnull));
void swap_HFSPlusAttrRecord     (HFSPlusAttrRecord* record) __attribute__((nonnull));

#endif
