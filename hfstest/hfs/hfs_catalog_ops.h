//
//  hfs_catalog_ops.h
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_catalog_ops_h
#define hfsinspect_hfs_catalog_ops_h

#include "hfs_structs.h"

HFSBTree    hfs_get_catalog_btree       (const HFSVolume *hfs);
u_int16_t   hfs_get_catalog_record_type (const HFSBTreeNode *node, hfs_record_id i);
int         hfs_get_catalog_leaf_record (HFSPlusCatalogKey* const record_key, void const** record_value, const HFSBTreeNode* node, hfs_record_id recordID);

int8_t      hfs_catalog_find_record     (HFSBTreeNode *node, hfs_record_id *recordID, const HFSVolume *hfs, hfs_node_id parentFolder, const wchar_t name[256], u_int8_t nameLength);
int         hfs_catalog_compare_keys_cf (const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2);
int         hfs_catalog_compare_keys_bc (const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2);

wchar_t*    hfsuctowcs                  (const HFSUniStr255* input);
HFSUniStr255 wcstohfsuc                 (const wchar_t* input);
HFSUniStr255 strtohfsuc                 (const char* input);


#endif
