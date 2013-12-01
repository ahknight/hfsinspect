//
//  attributes.h
//  hfsinspect
//
//  Created by Adam Knight on 6/16/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_attribute_ops_h
#define hfsinspect_hfs_attribute_ops_h

#include "hfs/hfs_structs.h"

int hfs_get_attribute_btree     (BTreePtr *tree, const HFS *hfs);
int hfs_attributes_compare_keys (const HFSPlusAttrKey *key1, const HFSPlusAttrKey *key2);
int hfs_attributes_get_node     (BTreeNodePtr *node, const BTreePtr bTree, bt_nodeid_t nodeNum);

void swap_HFSPlusAttrKey        (HFSPlusAttrKey *record);
void swap_HFSPlusAttrData       (HFSPlusAttrData* record);
void swap_HFSPlusAttrForkData   (HFSPlusAttrForkData* record);
void swap_HFSPlusAttrExtents    (HFSPlusAttrExtents* record);
void swap_HFSPlusAttrRecord     (HFSPlusAttrRecord* record);

#endif
