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
#include "hfs/hfs_structs.h"

// and a hat tip to Carbon ...
struct FSSpec {
    Volume          *vol;
    hfs_cnid_t      parID;
    HFSUniStr255    name;
};
typedef struct FSSpec FSSpec;
typedef FSSpec * FSSpecPtr;

extern const uint32_t kAliasCreator;
extern const uint32_t kFileAliasType;
extern const uint32_t kFolderAliasType;

extern wchar_t* HFSPlusMetadataFolder;
extern wchar_t* HFSPlusDirMetadataFolder;

int             hfs_get_catalog_btree                   (BTreePtr *tree, const HFS *hfs);
int             hfs_catalog_get_node                    (BTreeNodePtr *node, const BTreePtr bTree, bt_nodeid_t nodeNum);
//uint16_t        hfs_get_catalog_record_type             (const BTreeRecord* catalogRecord) __deprecated;
int             hfs_get_catalog_leaf_record             (HFSPlusCatalogKey* const record_key, HFSPlusCatalogRecord* const record_value, const BTreeNodePtr node, BTRecNum recordID);

int8_t          hfs_catalog_find_record                 (BTreeNodePtr* node, BTRecNum *recordID, const HFS *hfs, bt_nodeid_t parentFolder, HFSUniStr255 name);
int             hfs_catalog_compare_keys_cf             (const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2);
int             hfs_catalog_compare_keys_bc             (const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2);

bool            hfs_catalog_record_is_file_hard_link    (const HFSPlusCatalogRecord* record);
bool            hfs_catalog_record_is_directory_hard_link(const HFSPlusCatalogRecord* record);
bool            hfs_catalog_record_is_hard_link         (const HFSPlusCatalogRecord* record);
bool            hfs_catalog_record_is_symbolic_link     (const HFSPlusCatalogRecord* record);
bool            hfs_catalog_record_is_file_alias        (const HFSPlusCatalogRecord* record);
bool            hfs_catalog_record_is_directory_alias   (const HFSPlusCatalogRecord* record);
bool            hfs_catalog_record_is_alias             (const HFSPlusCatalogRecord* record);

//int             hfs_catalog_target_of_catalog_record    (bt_nodeid_t* nodeID, BTRecNum* recordID, const BTreeRecord* nodeRecord) __deprecated;
//BTreeRecord*    hfs_catalog_next_in_folder              (const BTreeRecord* catalogRecord) __deprecated;
//wchar_t*        hfs_catalog_record_to_path              (const BTreeRecord* catalogRecord) __deprecated;
int             hfs_catalog_get_cnid_name               (hfs_wc_str name, const HFS *hfs, bt_nodeid_t cnid);

int HFSPlusGetCatalogInfo(hfs_cnid_t *parent, HFSUniStr255 *name, HFSPlusCatalogRecord *catalogRecord, const char *path, const HFS *hfs)
__attribute__((nonnull(4,5)));

int             hfsuctowcs                              (hfs_wc_str output, const HFSUniStr255* input);
HFSUniStr255    wcstohfsuc                              (const wchar_t* input);
HFSUniStr255    strtohfsuc                              (const char* input);


void swap_HFSPlusBSDInfo            (HFSPlusBSDInfo *record);
void swap_FndrDirInfo               (FndrDirInfo *record);
void swap_FndrFileInfo              (FndrFileInfo *record);
void swap_FndrOpaqueInfo            (FndrOpaqueInfo *record);
void swap_HFSPlusCatalogKey         (HFSPlusCatalogKey *key);
void swap_HFSPlusCatalogRecord      (HFSPlusCatalogRecord *record);
void swap_HFSPlusCatalogFile        (HFSPlusCatalogFile *record);
void swap_HFSPlusCatalogFolder      (HFSPlusCatalogFolder *record);
void swap_HFSPlusCatalogThread      (HFSPlusCatalogThread *record);

#endif
