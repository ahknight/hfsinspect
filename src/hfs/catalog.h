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

#pragma mark High Level Commands

/** Straight lookup of the FSSpec as a key. Does not traverse threads. */
int HFSPlusGetCatalogRecordByFSSpec     (HFSPlusCatalogRecord *catalogRecord, FSSpec spec) _NONNULL;

/** Looks up a CNID and returns a file or folder.  Follows thread records. */
int HFSPlusGetCatalogInfoByCNID       (FSSpecPtr spec, HFSPlusCatalogRecord *catalogRecord, const HFS *hfs, bt_nodeid_t cnid) NONNULL(3);

/** Follows all thread records from the root to the given path. Returns a file/folder record, or a folder thread if the final component is a folder without a trailing slash. */
int HFSPlusGetCatalogInfoByPath         (FSSpecPtr spec, HFSPlusCatalogRecord *catalogRecord, const char *path, const HFS *hfs) NONNULL(3,4);

/** Looks up the thread record for the CNID, follows it to the record, then returns the name of that record. */
int HFSPlusGetCNIDName                  (wchar_t* name, FSSpec spec) _NONNULL;

/** Tries to follow all possible references from a catalog record, but only once. Returns 1 if the FSSpec refers to a new record, 0 if the source was not a reference, and -1 on error. */
int HFSPlusGetTargetOfCatalogRecord     (FSSpec *targetSpec, const HFSPlusCatalogRecord *sourceRecord, const HFS *hfs);

_NONNULL    bool    HFSPlusCatalogFileIsHardLink            (const HFSPlusCatalogRecord* record);
_NONNULL    bool    HFSPlusCatalogFolderIsHardLink          (const HFSPlusCatalogRecord* record);
_NONNULL    bool    HFSPlusCatalogRecordIsHardLink          (const HFSPlusCatalogRecord* record);
_NONNULL    bool    HFSPlusCatalogRecordIsSymLink           (const HFSPlusCatalogRecord* record);
_NONNULL    bool    HFSPlusCatalogRecordIsFileAlias         (const HFSPlusCatalogRecord* record);
_NONNULL    bool    HFSPlusCatalogRecordIsFolderAlias       (const HFSPlusCatalogRecord* record);
_NONNULL    bool    HFSPlusCatalogRecordIsAlias             (const HFSPlusCatalogRecord* record);

HFSPlusCatalogKey     HFSPlusCatalogKeyFromFSSpec           (FSSpec spec);
FSSpec                HFSPlusFSSpecFromCatalogKey           (HFSPlusCatalogKey key);

#pragma mark Low Level Commands

_NONNULL    int     hfs_get_catalog_btree                   (BTreePtr *tree, const HFS *hfs);
_NONNULL    int     hfs_catalog_get_node                    (BTreeNodePtr *node, const BTreePtr bTree, bt_nodeid_t nodeNum);
// NONNULL(3)    int     hfs_get_catalog_leaf_record             (HFSPlusCatalogKey* const record_key, HFSPlusCatalogRecord* const record_value, const BTreeNodePtr node, BTRecNum recordID) __deprecated;

_NONNULL    int8_t  hfs_catalog_find_record                 (BTreeNodePtr* node, BTRecNum *recordID, FSSpec spec);
_NONNULL    int     hfs_catalog_compare_keys_cf             (const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2);
_NONNULL    int     hfs_catalog_compare_keys_bc             (const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2);

#pragma mark Endian Madness

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
