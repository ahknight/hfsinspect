//
//  attributes.h
//  hfsinspect
//
//  Created by Adam Knight on 6/16/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_attribute_ops_h
#define hfsinspect_hfs_attribute_ops_h

#include "hfs/types.h"

#define _NONNULL __attribute__((nonnull))

typedef struct _HFSPlusAttributeName {
    u_int16_t attrNameLen;                  /* number of unicode characters */
    u_int16_t attrName[kHFSMaxAttrNameLen]; /* attribute name (Unicode) */
} HFSPlusAttributeName;

int hfsplus_get_attribute_btree     (BTreePtr* tree, const HFSPlus* hfs) _NONNULL;
int hfsplus_attributes_compare_keys (const HFSPlusAttrKey* key1, const HFSPlusAttrKey* key2) _NONNULL;
int hfsplus_attributes_get_node     (BTreeNodePtr* node, const BTreePtr bTree, bt_nodeid_t nodeNum) _NONNULL;

void swap_HFSPlusAttrKey        (HFSPlusAttrKey* record) _NONNULL;
void swap_HFSPlusAttrData       (HFSPlusAttrData* record) _NONNULL;
void swap_HFSPlusAttrForkData   (HFSPlusAttrForkData* record) _NONNULL;
void swap_HFSPlusAttrExtents    (HFSPlusAttrExtents* record) _NONNULL;
void swap_HFSPlusAttrRecord     (HFSPlusAttrRecord* record) _NONNULL;

__attribute__((nonnull(4)))
int HFSPlusGetAttributeList(void* attrList, uint32_t* attrCount, uint32_t fileID, const HFSPlus* hfs);
int HFSPlusGetAttribute(
    void* data,
    size_t* length,
    uint32_t fileID,
    const HFSPlusAttributeName* name,
    const HFSPlus* hfs
) _NONNULL;

#endif
