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
#include "hfs/types.h"

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
int HFSPlusGetCatalogRecordByFSSpec (HFSPlusCatalogRecord *catalogRecord, FSSpec spec) __attribute__((nonnull));

/** Looks up a CNID and returns a file or folder.  Follows thread records. */
int HFSPlusGetCatalogInfoByCNID (FSSpecPtr spec, HFSPlusCatalogRecord *catalogRecord, const HFS *hfs, bt_nodeid_t cnid) __attribute__((nonnull(3)));

/** Follows all thread records from the root to the given path. Returns a file/folder record, or a folder thread if the final component is a folder without a trailing slash. */
int HFSPlusGetCatalogInfoByPath (FSSpecPtr spec, HFSPlusCatalogRecord *catalogRecord, const char *path, const HFS *hfs) __attribute__((nonnull(3,4)));

/** Looks up the thread record for the CNID, follows it to the record, then returns the name of that record. */
int HFSPlusGetCNIDName (wchar_t* name, FSSpec spec) __attribute__((nonnull));

/** Tries to follow all possible references from a catalog record, but only once. Returns 1 if the FSSpec refers to a new record, 0 if the source was not a reference, and -1 on error. */
int HFSPlusGetTargetOfCatalogRecord (FSSpec *targetSpec, const HFSPlusCatalogRecord *sourceRecord, const HFS *hfs);

bool HFSPlusCatalogFileIsHardLink       (const HFSPlusCatalogRecord* record) __attribute__((nonnull));
bool HFSPlusCatalogFolderIsHardLink     (const HFSPlusCatalogRecord* record) __attribute__((nonnull));
bool HFSPlusCatalogRecordIsHardLink     (const HFSPlusCatalogRecord* record) __attribute__((nonnull));
bool HFSPlusCatalogRecordIsSymLink      (const HFSPlusCatalogRecord* record) __attribute__((nonnull));
bool HFSPlusCatalogRecordIsFileAlias    (const HFSPlusCatalogRecord* record) __attribute__((nonnull));
bool HFSPlusCatalogRecordIsFolderAlias  (const HFSPlusCatalogRecord* record) __attribute__((nonnull));
bool HFSPlusCatalogRecordIsAlias        (const HFSPlusCatalogRecord* record) __attribute__((nonnull));

HFSPlusCatalogKey HFSPlusCatalogKeyFromFSSpec (FSSpec spec);
FSSpec            HFSPlusFSSpecFromCatalogKey (HFSPlusCatalogKey key);

#pragma mark Low Level Commands

int hfs_get_catalog_btree (BTreePtr *tree, const HFS *hfs) __attribute__((nonnull));
int hfs_catalog_get_node (BTreeNodePtr *node, const BTreePtr bTree, bt_nodeid_t nodeNum) __attribute__((nonnull));
// int hfs_get_catalog_leaf_record (HFSPlusCatalogKey* const record_key, HFSPlusCatalogRecord* const record_value, const BTreeNodePtr node, BTRecNum recordID) __deprecated;

int8_t  hfs_catalog_find_record     (BTreeNodePtr* node, BTRecNum *recordID, FSSpec spec) __attribute__((nonnull));
int     hfs_catalog_compare_keys_cf (const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2) __attribute__((nonnull));
int     hfs_catalog_compare_keys_bc (const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2) __attribute__((nonnull));

#pragma mark Endian Madness

void swap_HFSPlusBSDInfo            (HFSPlusBSDInfo *record) __attribute__((nonnull));
void swap_FndrDirInfo               (FndrDirInfo *record) __attribute__((nonnull));
void swap_FndrFileInfo              (FndrFileInfo *record) __attribute__((nonnull));
void swap_FndrOpaqueInfo            (FndrOpaqueInfo *record) __attribute__((nonnull));
void swap_HFSPlusCatalogKey         (HFSPlusCatalogKey *key) __attribute__((nonnull));
void swap_HFSPlusCatalogRecord      (HFSPlusCatalogRecord *record) __attribute__((nonnull));
void swap_HFSPlusCatalogFile        (HFSPlusCatalogFile *record) __attribute__((nonnull));
void swap_HFSPlusCatalogFolder      (HFSPlusCatalogFolder *record) __attribute__((nonnull));
void swap_HFSPlusCatalogThread      (HFSPlusCatalogThread *record) __attribute__((nonnull));

#endif
