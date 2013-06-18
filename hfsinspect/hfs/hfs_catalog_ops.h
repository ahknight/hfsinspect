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

extern const u_int32_t kAliasCreator;
extern const u_int32_t kFileAliasType;
extern const u_int32_t kFolderAliasType;

extern wchar_t* HFSPlusMetadataFolder;
extern wchar_t* HFSPlusDirMetadataFolder;

union HFSPlusCatalogRecord {
    int16_t                 record_type;
    HFSPlusCatalogFile      catalogFile;
    HFSPlusCatalogFolder    catalogFolder;
    HFSPlusCatalogThread    catalogThread;
};
typedef union HFSPlusCatalogRecord HFSPlusCatalogRecord;


HFSBTree            hfs_get_catalog_btree                   (const HFSVolume *hfs);
u_int16_t           hfs_get_catalog_record_type             (const HFSBTreeNodeRecord* catalogRecord);
int                 hfs_get_catalog_leaf_record             (HFSPlusCatalogKey* const record_key, HFSPlusCatalogRecord* const record_value, const HFSBTreeNode* node, hfs_record_id recordID);

int8_t              hfs_catalog_find_record                 (HFSBTreeNode *node, hfs_record_id *recordID, const HFSVolume *hfs, hfs_node_id parentFolder, HFSUniStr255 name);
int                 hfs_catalog_compare_keys_cf             (const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2);
int                 hfs_catalog_compare_keys_bc             (const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2);

bool                hfs_catalog_record_is_hard_link         (HFSPlusCatalogRecord* record);
bool                hfs_catalog_record_is_symbolic_link     (HFSPlusCatalogRecord* record);
bool                hfs_catalog_record_is_alias             (HFSPlusCatalogRecord* record);

HFSBTreeNodeRecord* hfs_catalog_target_of_catalog_record    (const HFSBTreeNodeRecord* catalogRecord);
HFSBTreeNodeRecord* hfs_catalog_next_in_folder              (const HFSBTreeNodeRecord* catalogRecord);
wchar_t*            hfs_catalog_record_to_path              (const HFSBTreeNodeRecord* catalogRecord);
wchar_t*            hfs_catalog_get_cnid_name               (const HFSVolume *hfs, hfs_node_id cnid);

wchar_t*            hfsuctowcs                              (const HFSUniStr255* input);
HFSUniStr255        wcstohfsuc                              (const wchar_t* input);
HFSUniStr255        strtohfsuc                              (const char* input);


#endif
