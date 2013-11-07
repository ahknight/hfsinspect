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
#include "hfs_pstruct.h"
#include "hfs_unicode.h" // Apple's FastUnicodeCompare and conversion table.

const u_int32_t kAliasCreator       = 'MACS';
const u_int32_t kFileAliasType      = 'alis';
const u_int32_t kFolderAliasType    = 'fdrp';

wchar_t* HFSPlusMetadataFolder = L"\0\0\0\0HFS+ Private Data";
wchar_t* HFSPlusDirMetadataFolder = L".HFS+ Private Directory Data\xd";

HFSBTree hfs_get_catalog_btree(const HFSVolume *hfs)
{
    debug("Getting catalog B-Tree");
    
    static HFSBTree tree;
    if (tree.fork.cnid == 0) {
        debug("Creating catalog B-Tree");
        
        HFSFork fork = hfsfork_make(hfs, hfs->vh.catalogFile, HFSDataForkType, kHFSCatalogFileID);
        
        hfs_btree_init(&tree, &fork);
        
        if (hfs->vh.signature == kHFSXSigWord) {
            if (tree.headerRecord.keyCompareType == kHFSCaseFolding) {
                // Case Folding (normal; case-insensitive)
                tree.keyCompare = (hfs_compare_keys)hfs_catalog_compare_keys_cf;
                
            } else if (tree.headerRecord.keyCompareType == kHFSBinaryCompare) {
                // Binary Compare (case-sensitive)
                tree.keyCompare = (hfs_compare_keys)hfs_catalog_compare_keys_bc;
                
            }
        } else {
            // Case Folding (normal; case-insensitive)
            tree.keyCompare = (hfs_compare_keys)hfs_catalog_compare_keys_cf;
        }
    }
    
    return tree;
}

// Return is the record type (eg. kHFSPlusFolderRecord) and references to the key and value structs.
int hfs_get_catalog_leaf_record(HFSPlusCatalogKey* const record_key, HFSPlusCatalogRecord* const record_value, const HFSBTreeNode* node, hfs_record_id recordID)
{
    if (recordID >= node->recordCount) {
        error("Requested record %d, which is beyond the range of node %d (%d)", recordID, node->recordCount, node->nodeNumber);
        return -1;
    }
    
    debug("Getting catalog leaf record %d of node %d", recordID, node->nodeNumber);
    
    if (node->nodeDescriptor.kind != kBTLeafNode) return 0;
    
    const HFSBTreeNodeRecord *record    = &node->records[recordID];
    uint16_t max_key_length = sizeof(HFSPlusCatalogKey);
    uint16_t max_value_length = sizeof(HFSPlusCatalogRecord) + sizeof(uint16_t);
    
    u_int16_t key_length = record->keyLength;
    u_int16_t value_length = record->valueLength;
    
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


int8_t hfs_catalog_find_record(HFSBTreeNode *node, hfs_record_id *recordID, const HFSVolume *hfs, hfs_node_id parentFolder, HFSUniStr255 name)
{
    wchar_t* wc_name = hfsuctowcs(&name);
    debug("Searching catalog for %d:%ls", parentFolder, wc_name);
    FREE_BUFFER(wc_name);
    
    HFSPlusCatalogKey catalogKey;
    catalogKey.parentID = parentFolder;
    catalogKey.nodeName = name;
    catalogKey.keyLength = sizeof(catalogKey.parentID) + catalogKey.nodeName.length;
    catalogKey.keyLength = MAX(kHFSPlusCatalogKeyMinimumLength, catalogKey.keyLength);
    
    HFSBTree catalogTree = hfs_get_catalog_btree(hfs);
    HFSBTreeNode searchNode;
    hfs_record_id searchIndex = 0;
    
    bool result = hfs_btree_search_tree(&searchNode, &searchIndex, &catalogTree, &catalogKey);
    
    if (result == true) {
        *node = searchNode;
        *recordID = searchIndex;
        return 1;
    }
    
    return 0;
}

int hfs_catalog_compare_keys_cf(const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2)
{
//    if (getenv("DEBUG")) {
//        debug("compare catalog keys (CF)");
//        VisualizeHFSPlusCatalogKey(key1, "Search Key", 1);
//        VisualizeHFSPlusCatalogKey(key2, "Test Key  ", 1);
//    }
    if (key1->parentID == key2->parentID) {
        return FastUnicodeCompare(key1->nodeName.unicode, key1->nodeName.length, key2->nodeName.unicode, key2->nodeName.length);
    } else if (key1->parentID > key2->parentID) return 1;
    
    return -1;
}

int hfs_catalog_compare_keys_bc(const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2)
{
//    if (getenv("DEBUG")) {
//        debug("compare catalog keys (BC)");
//        VisualizeHFSPlusCatalogKey(key1, "Search Key", 1);
//        VisualizeHFSPlusCatalogKey(key2, "Test Key  ", 1);
//    }
    if (key1->parentID == key2->parentID) {
        wchar_t* key1Name = hfsuctowcs(&key1->nodeName);
        wchar_t* key2Name = hfsuctowcs(&key2->nodeName);
        int result = wcscmp(key1Name, key2Name);
        FREE_BUFFER(key1Name); FREE_BUFFER(key2Name);
        return result;
    } else if (key1->parentID > key2->parentID) return 1;
    
    return -1;
}

#pragma mark Searching

wchar_t* hfsuctowcs(const HFSUniStr255* input)
{
    // Get the length of the input
    int len = MIN(input->length, 255);
    
    // Allocate the return value
    wchar_t* output;
    INIT_STRING(output, sizeof(wchar_t) * (len+1));
    
    // Iterate over the input
    for (int i = 0; i < len; i++) {
        // Copy the u16 to the wchar array
        output[i] = input->unicode[i];
    }
    
    // Terminate the output at the length
    output[len] = L'\0';
    
    // Replace the catalog version with a printable version.
    if ( ( ! wcscmp(output, HFSPlusMetadataFolder)) ) {
        mbstowcs(output, HFSPLUSMETADATAFOLDER, len);
    }
    
    return output;
}

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
    INIT_STRING(wide, 256);
    
    size_t char_count = strlen(input);
    size_t wide_char_count = mbstowcs(wide, input, 255);
    if (wide_char_count > 0) output = wcstohfsuc(wide);
    
    if (char_count != wide_char_count) {
        error("Conversion error: mbstowcs returned a string of a different length than the input: %zd in; %zd out", char_count, wide_char_count);
    }
    FREE_BUFFER(wide);
    
    return output;
}

u_int16_t hfs_get_catalog_record_type (const HFSBTreeNodeRecord* catalogRecord)
{
    debug("Getting record type of %d:%d", catalogRecord->nodeID, catalogRecord->recordID);
    
    u_int16_t type = *(u_int16_t*)catalogRecord->value;
    
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

HFSBTreeNodeRecord* hfs_catalog_target_of_catalog_record (const HFSBTreeNodeRecord* nodeRecord)
{
    // Follow hard and soft links.
    HFSPlusCatalogRecord* catalogRecord = (HFSPlusCatalogRecord*)nodeRecord->value;
    u_int16_t kind = catalogRecord->record_type;
    
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
    return (HFSBTreeNodeRecord*)nodeRecord;
}

// Returned node is guaranteed to be in the same node for memory-management reasons (who releases the node?).
HFSBTreeNodeRecord* hfs_catalog_next_in_folder (const HFSBTreeNodeRecord* catalogRecord)
{
    // See if next record is the same parent ID. Return if so, otherwise NULL.
    HFSBTreeNode *node = catalogRecord->node;
    hfs_record_id recordID = catalogRecord->recordID;
    HFSBTreeNodeRecord* nextRecord = NULL;
    
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

HFSBTreeNodeRecord* hfs_catalog_path_to_record(const HFSVolume* hfs, const wchar_t* path)
{
    return NULL;
}

wchar_t* hfs_catalog_record_to_path (const HFSBTreeNodeRecord* catalogRecord)
{
    // Crawl parent ID up, gathering names along the way. Build path from that.
    return NULL;
}

wchar_t* hfs_catalog_get_cnid_name(const HFSVolume *hfs, hfs_node_id cnid)
{
    HFSBTreeNode node; memset(&node, 0, sizeof(node));
    hfs_record_id recordID = 0;
    HFSPlusCatalogKey key;
    key.parentID = cnid;
    key.nodeName.length = 0;
    key.nodeName.unicode[0] = '\0';
    key.nodeName.unicode[1] = '\0';
    
    HFSBTree tree = hfs_get_catalog_btree(hfs);
    
    int found = hfs_btree_search_tree(&node, &recordID, &tree, &key);
    if (found != 1 || node.buffer.size == 0) {
        error("No thread record for %d found.");
        return NULL;
    }
    
    debug("Found thread record %d:%d", node.nodeNumber, recordID);
    
    HFSPlusCatalogThread *threadRecord = (HFSPlusCatalogThread*)node.records[recordID].value;
    return hfsuctowcs(&threadRecord->nodeName);
}
