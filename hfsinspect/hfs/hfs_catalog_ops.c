//
//  hfs_catalog_ops.c
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs_catalog_ops.h"
#include "hfs_btree.h"
#include "hfs_io.h"
#include "output_hfs.h"
#include "hfs_unicode.h" // Apple's FastUnicodeCompare and conversion table.
#include <wchar.h>

const uint32_t kAliasCreator       = 'MACS';
const uint32_t kFileAliasType      = 'alis';
const uint32_t kFolderAliasType    = 'fdrp';

wchar_t* HFSPlusMetadataFolder = L"\0\0\0\0HFS+ Private Data";
wchar_t* HFSPlusDirMetadataFolder = L".HFS+ Private Directory Data\xd";

int hfs_get_catalog_btree(BTree* tree, const HFS *hfs)
{
    debug("Getting catalog B-Tree");
    
    static BTree* cachedTree;
    
    if (cachedTree == NULL) {
        debug("Creating catalog B-Tree");
        
        INIT_BUFFER(cachedTree, sizeof(BTree));

        HFSFork *fork;
        if ( hfsfork_get_special(&fork, hfs, kHFSCatalogFileID) < 0 ) {
            critical("Could not create fork for Catalog BTree!");
            return -1;
        }
        
        btree_init(cachedTree, fork);
        if (hfs->vh.signature == kHFSXSigWord) {
            if (cachedTree->headerRecord.keyCompareType == kHFSCaseFolding) {
                // Case Folding (normal; case-insensitive)
                cachedTree->keyCompare = (hfs_compare_keys)hfs_catalog_compare_keys_cf;
                
            } else if (cachedTree->headerRecord.keyCompareType == kHFSBinaryCompare) {
                // Binary Compare (case-sensitive)
                cachedTree->keyCompare = (hfs_compare_keys)hfs_catalog_compare_keys_bc;
                
            }
        } else {
            // Case Folding (normal; case-insensitive)
            cachedTree->keyCompare = (hfs_compare_keys)hfs_catalog_compare_keys_cf;
        }
    }
    
    // Copy the cached tree out.
    // Note this copies a reference to the same extent list in the HFSFork struct so NEVER free that fork.
    *tree = *cachedTree;
    
    return 0;
}

// Return is the record type (eg. kHFSPlusFolderRecord) and references to the key and value structs.
int hfs_get_catalog_leaf_record(HFSPlusCatalogKey* const record_key, HFSPlusCatalogRecord* const record_value, const BTreeNode* node, bt_recordid_t recordID)
{
    if (recordID >= node->recordCount) {
        error("Requested record %d, which is beyond the range of node %d (%d)", recordID, node->recordCount, node->nodeNumber);
        return -1;
    }
    
    debug("Getting catalog leaf record %d of node %d", recordID, node->nodeNumber);
    
    if (node->nodeDescriptor.kind != kBTLeafNode) return 0;
    
    const BTreeRecord *record = &node->records[recordID];
    uint16_t max_key_length = sizeof(HFSPlusCatalogKey);
    uint16_t max_value_length = sizeof(HFSPlusCatalogRecord) + sizeof(uint16_t);
    
    uint16_t key_length = record->keyLength;
    uint16_t value_length = record->valueLength;
    
    if (key_length > max_key_length) {
        warning("Key length of record %d in node %d is invalid (%d; maximum is %d)", recordID, node->nodeNumber, key_length, max_key_length);
        key_length = max_key_length;
    }
    if (value_length > max_value_length) {
        warning("Value length of record %d in node %d is invalid (%d; maximum is %d)", recordID, node->nodeNumber, value_length, max_value_length);
        value_length = max_value_length;
    }
    
    if (record->key != NULL) memcpy(record_key, record->key, key_length);
    if (record->value != NULL) memcpy(record_value, record->value, value_length);
    
    return record_value->record_type;
}


int8_t hfs_catalog_find_record(BTreeNode** node, bt_recordid_t *recordID, const HFS *hfs, bt_nodeid_t parentFolder, HFSUniStr255 name)
{
    hfs_wc_str wc_name;
    hfsuctowcs(wc_name, &name);
    debug("Searching catalog for %d:%ls", parentFolder, wc_name);
    
    HFSPlusCatalogKey catalogKey;
    catalogKey.parentID = parentFolder;
    catalogKey.nodeName = name;
    catalogKey.keyLength = sizeof(catalogKey.parentID) + catalogKey.nodeName.length;
    catalogKey.keyLength = MAX(kHFSPlusCatalogKeyMinimumLength, catalogKey.keyLength);
    
    BTree catalogTree;
    hfs_get_catalog_btree(&catalogTree, hfs);
    BTreeNode* searchNode;
    bt_recordid_t searchIndex = 0;
    
    bool result = btree_search_tree(&searchNode, &searchIndex, &catalogTree, &catalogKey);
    
    if (result == true) {
        *node = searchNode;
        *recordID = searchIndex;
        return 1;
    }
    
    return 0;
}

int hfs_catalog_compare_keys_cf(const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2)
{
    int result;
    if ( (result = cmp(key1->parentID, key2->parentID)) != 0) return result;
    
    result = FastUnicodeCompare(key1->nodeName.unicode, key1->nodeName.length, key2->nodeName.unicode, key2->nodeName.length);
    
    return result;
}

int hfs_catalog_compare_keys_bc(const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2)
{
    int result;
    if ( (result = cmp(key1->parentID, key2->parentID)) != 0) return result;
    
    hfs_wc_str key1Name, key2Name;
    hfsuctowcs(key1Name, &key1->nodeName);
    hfsuctowcs(key2Name, &key2->nodeName);
    result = wcscmp(key1Name, key2Name);
    
    return result;
}

#pragma mark Searching

int hfsuctowcs(hfs_wc_str output, const HFSUniStr255* input)
{
    // Get the length of the input
    int len = MIN(input->length, 255);
    
    // Copy the u16 to the wchar array
    FOR_UNTIL(i, len) output[i] = input->unicode[i];
    
    // Terminate the output at the length
    output[len] = L'\0';
    
    // Replace the catalog version with a printable version.
    if ( wcscmp(output, HFSPlusMetadataFolder) == 0 )
        mbstowcs(output, HFSPLUSMETADATAFOLDER, len);
    
    return len;
}

// FIXME: Eats higher-order Unicode chars. Could be fun.
HFSUniStr255 wcstohfsuc(const wchar_t* input)
{
    // Allocate the return value
    HFSUniStr255 output;
    
    // Get the length of the input
    size_t len = MIN(wcslen(input), 255);
    
    // Iterate over the input
    for (int i = 0; i < len; i++) {
        // Copy the input to the output
        output.unicode[i] = input[i];
    }
    
    // Terminate the output at the length
    output.length = len;
    
    return output;
}

HFSUniStr255 strtohfsuc(const char* input)
{
    HFSUniStr255 output = {0, {'\0','\0'}};
    wchar_t* wide;
    INIT_BUFFER(wide, 256 * sizeof(wchar_t));
    
    size_t char_count = strlen(input);
    size_t wide_char_count = mbstowcs(wide, input, 255);
    if (wide_char_count > 0) output = wcstohfsuc(wide);
    
    if (char_count != wide_char_count) {
        error("Conversion error: mbstowcs returned a string of a different length than the input: %zd in; %zd out", char_count, wide_char_count);
    }
    FREE_BUFFER(wide);
    
    return output;
}

uint16_t hfs_get_catalog_record_type (const BTreeRecord* catalogRecord)
{
    debug("Getting record type of %d:%d", catalogRecord->nodeID, catalogRecord->recordID);
    
    uint16_t type = *(uint16_t*)catalogRecord->value;
    
    return type;
}


#pragma mark Fetch and List

bool hfs_catalog_record_is_file_hard_link(const HFSPlusCatalogRecord* record)
{
    return (
            (record->record_type == kHFSPlusFileRecord) &&
            (record->catalogFile.userInfo.fdCreator == kHFSPlusCreator && record->catalogFile.userInfo.fdType == kHardLinkFileType)
            );
}

bool hfs_catalog_record_is_directory_hard_link(const HFSPlusCatalogRecord* record)
{
    return (
            (record->record_type == kHFSPlusFileRecord) &&
            ( (record->catalogFile.flags & kHFSHasLinkChainMask) && hfs_catalog_record_is_directory_alias(record) )
            );
}

bool hfs_catalog_record_is_hard_link(const HFSPlusCatalogRecord* record)
{
    return ( hfs_catalog_record_is_file_hard_link(record) || hfs_catalog_record_is_directory_hard_link(record) );
}

bool hfs_catalog_record_is_symbolic_link(const HFSPlusCatalogRecord* record)
{
    return (
            (record->record_type == kHFSPlusFileRecord) &&
            (record->catalogFile.userInfo.fdCreator == kSymLinkCreator) &&
            (record->catalogFile.userInfo.fdType == kSymLinkFileType)
            );
}

bool hfs_catalog_record_is_file_alias(const HFSPlusCatalogRecord* record)
{
    return (
            (record->record_type == kHFSPlusFileRecord) &&
//            (record->catalogFile.userInfo.fdFlags & kIsAlias) &&
            (record->catalogFile.userInfo.fdCreator == kAliasCreator) &&
            (record->catalogFile.userInfo.fdType == kFileAliasType)
            );
}

bool hfs_catalog_record_is_directory_alias(const HFSPlusCatalogRecord* record)
{
    return (
            (record->record_type == kHFSPlusFileRecord) &&
//            (record->catalogFile.userInfo.fdFlags & kIsAlias) &&
            (record->catalogFile.userInfo.fdCreator == kAliasCreator) &&
            (record->catalogFile.userInfo.fdType == kFolderAliasType)
            );
}

bool hfs_catalog_record_is_alias(const HFSPlusCatalogRecord* record)
{
    return ( hfs_catalog_record_is_file_alias(record) || hfs_catalog_record_is_directory_alias(record) );
}

int hfs_catalog_target_of_catalog_record (bt_nodeid_t* nodeID, bt_recordid_t* recordID, const BTreeRecord* nodeRecord)
{
    // Follow hard and soft links.
    HFSPlusCatalogRecord* catalogRecord = (HFSPlusCatalogRecord*)nodeRecord->value;
    uint16_t kind = catalogRecord->record_type;
    
    if (kind == kHFSFileRecord) {
        // Hard link
        if (hfs_catalog_record_is_hard_link(catalogRecord)) {
            print("Record is a hard link.");
//            hfs_node_id destinationCNID = catalogRecord->catalogFile.bsdInfo.special.iNodeNum;
            // TODO: fetch CNID
        }
        
        // Soft link
        if (hfs_catalog_record_is_symbolic_link(catalogRecord)) {
            print("Record is a symbolic link.");
            // TODO: read data fork for volume-absolute path
        }
        
    }
    
    // If this isn't a link, then the right answer is "itself".
    *nodeID = nodeRecord->nodeID;
    *recordID = nodeRecord->recordID;
    
    return 0;
}

// Returned node is guaranteed to be in the same node for memory-management reasons (who releases the node?).
BTreeRecord* hfs_catalog_next_in_folder (const BTreeRecord* catalogRecord)
{
    // See if next record is the same parent ID. Return if so, otherwise NULL.
    BTreeNode *node = catalogRecord->node;
    bt_recordid_t recordID = catalogRecord->recordID;
    BTreeRecord* nextRecord = NULL;
    
    if ( (recordID + 1) < node->recordCount) {
        nextRecord = &node->records[recordID + 1];
    } else {
        debug("There is no next record in this node (%d). Follow fLink and try again.", node->nodeNumber);
        return NULL; // Follow fLink yourself if (recordID+1 >= recordCount) so you can release the node.
    }
    
    if ( ((HFSPlusCatalogKey*)catalogRecord->key)->parentID == ((HFSPlusCatalogKey*)nextRecord->key)->parentID ) {
        return nextRecord;
    }
    
    return NULL;
}

BTreeRecord* hfs_catalog_path_to_record(const HFS* hfs, const wchar_t* path)
{
    return NULL;
}

wchar_t* hfs_catalog_record_to_path (const BTreeRecord* catalogRecord)
{
    // Crawl parent ID up, gathering names along the way. Build path from that.
    return NULL;
}

int hfs_catalog_get_cnid_name(hfs_wc_str name, const HFS *hfs, bt_nodeid_t cnid)
{
    BTree tree;
    BTreeNode* node = NULL;
    bt_recordid_t recordID = 0;
    HFSPlusCatalogKey key = {0};
    
    key.parentID = cnid;
//    key.nodeName.length = 0;
//    key.nodeName.unicode[0] = '\0';
//    key.nodeName.unicode[1] = '\0';
    
    hfs_get_catalog_btree(&tree, hfs);
    
    int found = btree_search_tree(&node, &recordID, &tree, &key);
    if (found != 1 || node->dataLen == 0) {
        error("No thread record for %d found.");
        return NULL;
    }
    
    debug("Found thread record %d:%d", node->nodeNumber, recordID);
    
    HFSPlusCatalogThread *threadRecord = (HFSPlusCatalogThread*)node->records[recordID].value;
    hfsuctowcs(name, &threadRecord->nodeName);
    
    return wcslen(name);
}
