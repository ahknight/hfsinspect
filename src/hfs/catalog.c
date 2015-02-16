//
//  catalog.c
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <string.h>             // memcpy, strXXX, etc.
#if defined(__linux__)
    #include <bsd/string.h>     // strlcpy, etc.
#endif

#include <errno.h>              // errno/perror
#include <wchar.h>
#include <assert.h>             // assert()

#include "hfs/catalog.h"

#include "hfs/hfs_io.h"
#include "hfs/output_hfs.h"
#include "hfs/Apple/hfs_types.h"   // Apple's FastUnicodeCompare and conversion table.
#include "hfs/hfs_endian.h"
#include "hfs/unicode.h"
#include "volumes/utilities.h"     // commonly-used utility functions
#include "logging/logging.h"       // console printing routines


const uint32_t kAliasCreator            = 'MACS';
const uint32_t kFileAliasType           = 'alis';
const uint32_t kFolderAliasType         = 'fdrp';

wchar_t*       HFSPlusMetadataFolder    = L"\0\0\0\0HFS+ Private Data";
wchar_t*       HFSPlusDirMetadataFolder = L".HFS+ Private Directory Data\xd";

int hfs_get_catalog_btree(BTreePtr* tree, const HFS* hfs)
{
    trace("tree (%p), hfs (%p)", tree, hfs);
    debug("Getting catalog B-Tree");

    static BTreePtr cachedTree;

    if (cachedTree == NULL) {
        HFSFork* fork = NULL;
        FILE*    fp   = NULL;

        debug("Creating catalog B-Tree");

        SALLOC(cachedTree, sizeof(struct _BTree));

        if ( hfsfork_get_special(&fork, hfs, kHFSCatalogFileID) < 0 ) {
            critical("Could not create fork for Catalog B-Tree!");
        }

        fp = fopen_hfsfork(fork);
        if (fp == NULL) return -1;

        btree_init(cachedTree, fp);
        if (hfs->vh.signature == kHFSXSigWord) {
            if (cachedTree->headerRecord.keyCompareType == kHFSCaseFolding) {
                // Case Folding (normal; case-insensitive)
                cachedTree->keyCompare = (btree_key_compare_func)hfs_catalog_compare_keys_cf;

            } else if (cachedTree->headerRecord.keyCompareType == kHFSBinaryCompare) {
                // Binary Compare (case-sensitive)
                cachedTree->keyCompare = (btree_key_compare_func)hfs_catalog_compare_keys_bc;

            }
        } else {
            // Case Folding (normal; case-insensitive)
            cachedTree->keyCompare = (btree_key_compare_func)hfs_catalog_compare_keys_cf;
        }
        cachedTree->treeID  = kHFSCatalogFileID;
        cachedTree->getNode = hfs_catalog_get_node;

        // Load the bitmap.
        (void)BTIsNodeUsed(cachedTree, 0);
    }

    // Copy the cached tree out.
    // Note this copies a reference to the same extent list in the HFSFork struct so NEVER free that fork.
    *tree = cachedTree;

    return 0;
}

int hfs_catalog_get_node(BTreeNodePtr* out_node, const BTreePtr bTree, bt_nodeid_t nodeNum)
{
    trace("out_node (%p), bTree (%p), nodeNum %u", out_node, bTree, nodeNum);
    assert(out_node);
    assert(bTree);
    assert(bTree->treeID == kHFSCatalogFileID);

    BTreeNodePtr node = NULL;

    if ( btree_get_node(&node, bTree, nodeNum) < 0) return -1;

    if (node == NULL) {
        // empty node
        return 0;
    }

    // Swap catalog-specific structs in the records
    if ((node->nodeDescriptor->kind == kBTIndexNode) || (node->nodeDescriptor->kind == kBTLeafNode)) {
        for (unsigned recNum = 0; recNum < node->recordCount; recNum++) {
            void*              record     = NULL;
            HFSPlusCatalogKey* catalogKey = NULL;
            BTRecOffset        keyLen     = 0;

            // Get the raw record
            record     = BTGetRecord(node, recNum);

            // Swap the key first since we need a correct key length
            catalogKey = (HFSPlusCatalogKey*)record;
            swap_HFSPlusCatalogKey(catalogKey);

            // Verify key
            if (catalogKey->nodeName.length > 255) {
                warning("Record %d in node %d has an invalid name length: %d", recNum, nodeNum, catalogKey->nodeName.length);
                catalogKey->nodeName.length = 255; // Not the right answer, but better than reading 8K of data later on.
            }

            // Index nodes are already swapped, so only swap leaf nodes
            if (node->nodeDescriptor->kind == kBTLeafNode) {
                void* catalogRecord = NULL;
                keyLen        = BTGetRecordKeyLength(node, recNum);
                catalogRecord = ((char*)record + keyLen);
                swap_HFSPlusCatalogRecord((HFSPlusCatalogRecord*)catalogRecord);
            }
        }
    }

    *out_node = node;

    return 0;
}

// Return is the record type (eg. kHFSPlusFolderRecord) and references to the key and value structs.
// int hfs_get_catalog_leaf_record(HFSPlusCatalogKey* const record_key, HFSPlusCatalogRecord* const record_value, const BTreeNodePtr node, BTRecNum recordID)
// {
//     if (recordID >= node->recordCount) {
//         error("Requested record %d, which is beyond the range of node %d (%d)", recordID, node->recordCount, node->nodeNumber);
//         return -1;
//     }
//
//     debug("Getting catalog leaf record %d of node %d", recordID, node->nodeNumber);
//
//     if (node->nodeDescriptor->kind != kBTLeafNode) return 0;
//
//     BTNodeRecord record = {0};
//     if (BTGetBTNodeRecord(&record, node, recordID) < 0)
//         return -1;
//
//     uint16_t max_key_length = sizeof(HFSPlusCatalogKey);
//     uint16_t max_value_length = sizeof(HFSPlusCatalogRecord) + sizeof(uint16_t);
//
//     uint16_t key_length = record.keyLen;
//     uint16_t value_length = record.valueLen;
//
//     if (key_length > max_key_length) {
//         warning("Key length of record %d in node %d is invalid (%d; maximum is %d)", recordID, node->nodeNumber, key_length, max_key_length);
//     }
//     if (value_length > max_value_length) {
//         warning("Value length of record %d in node %d is invalid (%d; maximum is %d)", recordID, node->nodeNumber, value_length, max_value_length);
//     }
//
//     if (record_key != NULL)     *record_key = *(HFSPlusCatalogKey*)record.key;
//     if (record_value != NULL)   *record_value = *(HFSPlusCatalogRecord*)record.value;
//
//     return ((HFSPlusCatalogRecord*)record.value)->record_type;
// }

int8_t hfs_catalog_find_record(BTreeNodePtr* node, BTRecNum* recordID, FSSpec spec)
{
    hfs_wc_str   wc_name      = {0};
    const HFS*   hfs          = spec.hfs;
    bt_nodeid_t  parentFolder = spec.parentID;
    HFSUniStr255 name         = spec.name;

    trace("node (%p), recordID (%p), spec (%p, %u, (%u))", node, recordID, spec.hfs, spec.parentID, spec.name.length);

    hfsuctowcs(wc_name, &name);
    debug("Searching catalog for %d:%ls", parentFolder, wc_name);

    HFSPlusCatalogKey catalogKey = {0};
    catalogKey.parentID  = parentFolder;
    catalogKey.nodeName  = name;
    catalogKey.keyLength = sizeof(catalogKey.parentID) + catalogKey.nodeName.length;
    catalogKey.keyLength = MAX(kHFSPlusCatalogKeyMinimumLength, catalogKey.keyLength);

    BTreePtr     catalogTree = NULL;
    hfs_get_catalog_btree(&catalogTree, hfs);
    BTreeNodePtr searchNode  = NULL;
    BTRecNum     searchIndex = 0;

    bool         result      = btree_search(&searchNode, &searchIndex, catalogTree, &catalogKey);

    if (result == true) {
        *node     = searchNode;
        *recordID = searchIndex;
        return 1;
    }

    return 0;
}

int hfs_catalog_compare_keys_cf(const HFSPlusCatalogKey* key1, const HFSPlusCatalogKey* key2)
{
    int result = 0;

    trace("key1 (%p) (%u, %u), key2 (%p) (%u, %u)",
          key1, key1->keyLength, key1->parentID,
          key2, key2->keyLength, key2->parentID);

    if ( (result = cmp(key1->parentID, key2->parentID)) != 0) return result;

    result = FastUnicodeCompare(key1->nodeName.unicode, key1->nodeName.length, key2->nodeName.unicode, key2->nodeName.length);

    return result;
}

int hfs_catalog_compare_keys_bc(const HFSPlusCatalogKey* key1, const HFSPlusCatalogKey* key2)
{
    int result = 0;

    trace("key1 (%p) (%u, %u), key2 (%p) (%u, %u)",
          key1, key1->keyLength, key1->parentID,
          key2, key2->keyLength, key2->parentID);

    if ( (result = cmp(key1->parentID, key2->parentID)) != 0) return result;

    hfs_wc_str key1Name, key2Name;
    hfsuctowcs(key1Name, &key1->nodeName);
    hfsuctowcs(key2Name, &key2->nodeName);
    result = wcscmp(key1Name, key2Name);

    return result;
}

int HFSPlusGetCNIDName(wchar_t* name, FSSpec spec)
{
    const HFS*        hfs      = spec.hfs;
    bt_nodeid_t       cnid     = spec.parentID;
    BTreePtr          tree     = NULL;
    BTreeNodePtr      node     = NULL;
    BTRecNum          recordID = 0;
    HFSPlusCatalogKey key      = {0};

    trace("name %ls, spec (%p, %u, (%u))", name, spec.hfs, spec.parentID, spec.name.length);

    key.parentID  = cnid;
    key.keyLength = kHFSPlusCatalogKeyMinimumLength;

    if (hfs_get_catalog_btree(&tree, hfs) < 0)
        return -1;

    int found = btree_search(&node, &recordID, tree, &key);
    if ((found != 1) || (node->dataLen == 0)) {
        warning("No thread record for %d found.", cnid);
        return -1;
    }

    debug("Found thread record %d:%d", node->nodeNumber, recordID);

    BTreeKeyPtr           recordKey    = NULL;
    void*                 recordValue  = NULL;
    btree_get_record(&recordKey, &recordValue, node, recordID);

    HFSPlusCatalogThread* threadRecord = (HFSPlusCatalogThread*)recordValue;
    hfsuctowcs(name, &threadRecord->nodeName);

    return wcslen(name);
}

#pragma mark Searching

/**
   FIXME: Incomplete
   Tries to follow all possible references from a catalog record, but only once. Returns 1 if the FSSpec refers to a new record, 0 if the source was not a reference, and -1 on error.
 */
//int HFSPlusGetTargetOfCatalogRecord(FSSpec* targetSpec, const HFSPlusCatalogRecord* sourceRecord, const HFS* hfs)
//{
//    trace("targetSpec (%p), sourceRecord (%p), hfs (%p)", targetSpec, sourceRecord, hfs);
//
//    switch(sourceRecord->record_type) {
//        case kHFSPlusFileRecord:
//        {
//            if (HFSPlusCatalogRecordIsHardLink(sourceRecord)) {
//                bt_nodeid_t targetCNID = sourceRecord->catalogFile.bsdInfo.special.iNodeNum;
//                FSSpec      spec       = {0};
//                if ( HFSPlusGetCatalogInfoByCNID(&spec, NULL, hfs, targetCNID) < 0)
//                    return -1;
//                *targetSpec = spec;
//                return 1;
//            }
//        }
//
//        case kHFSPlusFolderRecord:
//        {
//
//        }
//
//        case kHFSPlusFolderThreadRecord:
//        case kHFSPlusFileThreadRecord:
//        {
//
//        }
//
//        default:
//            return -1;
//    }
//    return 0;
//}

int HFSPlusGetCatalogInfoByPath(FSSpecPtr out_spec, HFSPlusCatalogRecord* out_catalogRecord, const char* path, const HFS* hfs)
{
    trace("out_spec (%p), out_catalogRecord (%p), path '%s', hfs (%p)", out_spec, out_catalogRecord, path, hfs);

    debug("Finding record for path: %s", path);

    if (strlen(path) == 0) {
        errno = EINVAL;
        return -1;
    }

    int                  found                  = 0;
    hfs_cnid_t           parentID               = kHFSRootFolderID, last_parentID = 0;
    char*                file_path, * dup_path;
    char*                segment                = NULL;
    char                 last_segment[PATH_MAX] = "";

    HFSPlusCatalogRecord catalogRecord          = {0};

    file_path = dup_path = strdup(path);

    while ( (segment = strsep(&file_path, "/")) != NULL ) {
        debug("Segment: %d:%s", parentID, segment);
        (void)strlcpy(last_segment, segment, PATH_MAX);
        last_parentID = parentID;

        FSSpec spec = { .hfs = hfs, .parentID = last_parentID, .name = strtohfsuc(segment) };
        found         = !(HFSPlusGetCatalogRecordByFSSpec(&catalogRecord, spec) < 0);
        if ( !found ) {
            debug("Record NOT found:");
            debug("%s: segment '%u:%s' failed", path, parentID, segment);
            break;
        }

        debug("Record found:");
        debug("%s: parent: %d ", segment, parentID);
        if (strlen(segment) == 0) continue;

        switch (catalogRecord.record_type) {
            case kHFSPlusFolderRecord:
            {
                parentID = catalogRecord.catalogFolder.folderID;
                break;
            }

            case kHFSPlusFolderThreadRecord:
            case kHFSPlusFileThreadRecord:
            {
                parentID = catalogRecord.catalogThread.parentID;
                break;
            }

            case kHFSPlusFileRecord:
            {
                break;
            }

            default:
            {
                error("Didn't change the parent (%d).", catalogRecord.record_type);
                break;
            }
        }
    }

    if (found && (last_segment != NULL)) {
        if (out_spec != NULL) {
            out_spec->hfs      = hfs;
            out_spec->parentID = last_parentID;
            out_spec->name     = strtohfsuc(last_segment);
        }
        if (out_catalogRecord != NULL) *out_catalogRecord = catalogRecord;
    }

    SFREE(dup_path);

    debug("found: %u", found);
    return (found ? 0 : -1);
}

HFSPlusCatalogKey HFSPlusCatalogKeyFromFSSpec(FSSpec spec)
{
    trace("spec (%p, %u, (%u)", spec.hfs, spec.parentID, spec.name.length);

    HFSPlusCatalogKey catalogKey = {0};
    catalogKey.parentID  = spec.parentID;
    catalogKey.nodeName  = spec.name;
    catalogKey.keyLength = sizeof(catalogKey.parentID) + catalogKey.nodeName.length;
    catalogKey.keyLength = MAX(kHFSPlusCatalogKeyMinimumLength, catalogKey.keyLength);
    return catalogKey;
}

FSSpec HFSPlusFSSpecFromCatalogKey(HFSPlusCatalogKey key)
{
    trace("key (%u, %u)", key.keyLength, key.parentID);

    FSSpec spec = { .parentID = key.parentID, .name = key.nodeName };
    return spec;
}

int HFSPlusGetCatalogRecordByFSSpec(HFSPlusCatalogRecord* catalogRecord, FSSpec spec)
{
    const HFS*        hfs         = spec.hfs;
    HFSPlusCatalogKey catalogKey  = HFSPlusCatalogKeyFromFSSpec(spec);
    BTreePtr          catalogTree = NULL;
    BTreeNodePtr      searchNode  = NULL;
    BTRecNum          searchIndex = 0;
    bool              result      = false;
    BTNodeRecord      record      = {0};

    trace("catalogRecord (%p), spec (%p, %u, (%u)", catalogRecord, spec.hfs, spec.parentID, spec.name.length);

    if ( hfs_get_catalog_btree(&catalogTree, hfs) < 0 ) return -1;

    result = btree_search(&searchNode, &searchIndex, catalogTree, &catalogKey);
    if (result != true) return -1;

    if ( BTGetBTNodeRecord(&record, searchNode, searchIndex) < 0) {
        debug("Get record failed.");
        return -1;
    }

    *catalogRecord = *(HFSPlusCatalogRecord*)record.value; //copy

    btree_free_node(searchNode);

    debug("returning");
    return 0;
}

int HFSPlusGetCatalogInfoByCNID(FSSpec* out_spec, HFSPlusCatalogRecord* out_catalogRecord, const HFS* hfs, bt_nodeid_t cnid)
{
    FSSpec               spec          = { .hfs = hfs, .parentID = cnid };
    HFSPlusCatalogRecord catalogRecord = {0};

    trace("out_spec (%p), out_catalogRecord (%p), hfs (%p), cnid %u", out_spec, out_catalogRecord, hfs, cnid);

    if ( HFSPlusGetCatalogRecordByFSSpec(&catalogRecord, spec) < 0 )
        return -1;

    if ((catalogRecord.record_type == kHFSPlusFileThreadRecord) || (catalogRecord.record_type == kHFSPlusFolderThreadRecord)) {
        spec.parentID = catalogRecord.catalogThread.parentID;
        spec.name     = catalogRecord.catalogThread.nodeName;

        if ( HFSPlusGetCatalogRecordByFSSpec(&catalogRecord, spec) < 0 )
            return -1;
    }

    if (out_spec)           *out_spec = spec;
    if (out_catalogRecord)  *out_catalogRecord = catalogRecord;

    return 0;
}

#pragma mark Property Tests

bool HFSPlusCatalogFileIsHardLink(const HFSPlusCatalogRecord* record)
{
    trace("record (%p)", record);

    return (
        (record->record_type == kHFSPlusFileRecord) &&
        (record->catalogFile.userInfo.fdCreator == kHFSPlusCreator && record->catalogFile.userInfo.fdType == kHardLinkFileType)
        );
}

bool HFSPlusCatalogFolderIsHardLink(const HFSPlusCatalogRecord* record)
{
    trace("record (%p)", record);

    return (
        (record->record_type == kHFSPlusFileRecord) &&
        ( (record->catalogFile.flags & kHFSHasLinkChainMask) && HFSPlusCatalogRecordIsFolderAlias(record) )
        );
}

bool HFSPlusCatalogRecordIsHardLink(const HFSPlusCatalogRecord* record)
{
    trace("record (%p)", record);

    return ( HFSPlusCatalogFileIsHardLink(record) || HFSPlusCatalogFolderIsHardLink(record) );
}

bool HFSPlusCatalogRecordIsSymLink(const HFSPlusCatalogRecord* record)
{
    trace("record (%p)", record);

    return (
        (record->record_type == kHFSPlusFileRecord) &&
        (record->catalogFile.userInfo.fdCreator == kSymLinkCreator) &&
        (record->catalogFile.userInfo.fdType == kSymLinkFileType)
        );
}

bool HFSPlusCatalogRecordIsFileAlias(const HFSPlusCatalogRecord* record)
{
    trace("record (%p)", record);

    return (
        (record->record_type == kHFSPlusFileRecord) &&
//            (record->catalogFile.userInfo.fdFlags & kIsAlias) &&
        (record->catalogFile.userInfo.fdCreator == kAliasCreator) &&
        (record->catalogFile.userInfo.fdType == kFileAliasType)
        );
}

bool HFSPlusCatalogRecordIsFolderAlias(const HFSPlusCatalogRecord* record)
{
    trace("record (%p)", record);

    return (
        (record->record_type == kHFSPlusFileRecord) &&
//            (record->catalogFile.userInfo.fdFlags & kIsAlias) &&
        (record->catalogFile.userInfo.fdCreator == kAliasCreator) &&
        (record->catalogFile.userInfo.fdType == kFolderAliasType)
        );
}

bool HFSPlusCatalogRecordIsAlias(const HFSPlusCatalogRecord* record)
{
    trace("record (%p)", record);

    return ( HFSPlusCatalogRecordIsFileAlias(record) || HFSPlusCatalogRecordIsFolderAlias(record) );
}

