//
//  catalog.h
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_catalog_ops_h
#define hfsinspect_hfs_catalog_ops_h

#include "hfs/btree/btree.h"
#include "hfs/hfs_types.h"

// and a hat tip to Carbon ...
struct FSSpec {
    const HFS       *hfs;
    hfs_cnid_t      parentID;
    HFSUniStr255    name;
};
typedef struct FSSpec FSSpec;
typedef FSSpec * FSSpecPtr;

extern const uint32_t kAliasCreator;
extern const uint32_t kFileAliasType;
extern const uint32_t kFolderAliasType;

extern wchar_t* HFSPlusMetadataFolder;
extern wchar_t* HFSPlusDirMetadataFolder;

_NONNULL    int     hfs_get_catalog_btree                   (BTreePtr *tree, const HFS *hfs);
_NONNULL    int     hfs_catalog_get_node                    (BTreeNodePtr *node, const BTreePtr bTree, bt_nodeid_t nodeNum);
_NONNULL    int     hfs_get_catalog_leaf_record             (HFSPlusCatalogKey* const record_key, HFSPlusCatalogRecord* const record_value, const BTreeNodePtr node, BTRecNum recordID);

_NONNULL    int8_t  hfs_catalog_find_record                 (BTreeNodePtr* node, BTRecNum *recordID, FSSpec spec);
_NONNULL    int     hfs_catalog_compare_keys_cf             (const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2);
_NONNULL    int     hfs_catalog_compare_keys_bc             (const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2);

_NONNULL    bool    hfs_catalog_record_is_file_hard_link    (const HFSPlusCatalogRecord* record);
_NONNULL    bool    hfs_catalog_record_is_directory_hard_link(const HFSPlusCatalogRecord* record);
_NONNULL    bool    hfs_catalog_record_is_hard_link         (const HFSPlusCatalogRecord* record);
_NONNULL    bool    hfs_catalog_record_is_symbolic_link     (const HFSPlusCatalogRecord* record);
_NONNULL    bool    hfs_catalog_record_is_file_alias        (const HFSPlusCatalogRecord* record);
_NONNULL    bool    hfs_catalog_record_is_directory_alias   (const HFSPlusCatalogRecord* record);
_NONNULL    bool    hfs_catalog_record_is_alias             (const HFSPlusCatalogRecord* record);

//int             hfs_catalog_target_of_catalog_record    (bt_nodeid_t* nodeID, BTRecNum* recordID, const BTreeRecord* nodeRecord) __deprecated;
//BTreeRecord*    hfs_catalog_next_in_folder              (const BTreeRecord* catalogRecord) __deprecated;
//wchar_t*        hfs_catalog_record_to_path              (const BTreeRecord* catalogRecord) __deprecated;
_NONNULL    int     hfs_catalog_get_cnid_name               (hfs_wc_str name, const HFS *hfs, bt_nodeid_t cnid);

NONNULL(3,4)int HFSPlusGetCatalogInfo(FSSpecPtr spec, HFSPlusCatalogRecord *catalogRecord, const char *path, const HFS *hfs);

_NONNULL    int             hfsuctowcs                      (hfs_wc_str output, const HFSUniStr255* input);
_NONNULL    HFSUniStr255    wcstohfsuc                      (const wchar_t* input);
_NONNULL    HFSUniStr255    strtohfsuc                      (const char* input);


_NONNULL    void swap_HFSPlusBSDInfo            (HFSPlusBSDInfo *record);
_NONNULL    void swap_FndrDirInfo               (FndrDirInfo *record);
_NONNULL    void swap_FndrFileInfo              (FndrFileInfo *record);
_NONNULL    void swap_FndrOpaqueInfo            (FndrOpaqueInfo *record);
_NONNULL    void swap_HFSPlusCatalogKey         (HFSPlusCatalogKey *key);
_NONNULL    void swap_HFSPlusCatalogRecord      (HFSPlusCatalogRecord *record);
_NONNULL    void swap_HFSPlusCatalogFile        (HFSPlusCatalogFile *record);
_NONNULL    void swap_HFSPlusCatalogFolder      (HFSPlusCatalogFolder *record);
_NONNULL    void swap_HFSPlusCatalogThread      (HFSPlusCatalogThread *record);

#endif
