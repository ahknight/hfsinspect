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

HFSBTree hfs_get_catalog_btree(const HFSVolume *hfs)
{
    debug("Getting catalog B-Tree");
    
    static HFSBTree tree;
    if (tree.fork.cnid == 0) {
        debug("Creating catalog B-Tree");
        
        HFSFork fork = hfsfork_make(hfs, hfs->vh.catalogFile, HFSDataForkType, kHFSCatalogFileID);
        
        hfs_btree_init(&tree, &fork);
        
        if (tree.headerRecord.keyCompareType == 0xCF) {
            // Case Folding (normal; case-insensitive)
            tree.keyCompare = (hfs_compare_keys)hfs_catalog_compare_keys_cf;
            
        } else if (tree.headerRecord.keyCompareType == 0xBC) {
            // Binary Compare (case-sensitive)
            tree.keyCompare = (hfs_compare_keys)hfs_catalog_compare_keys_bc;
            
        }
    }
    
    return tree;
}

// Return is the record type (eg. kHFSPlusFolderRecord) and references to the key and value structs.
int hfs_get_catalog_leaf_record(HFSPlusCatalogKey* const record_key, void const** record_value, const HFSBTreeNode* node, hfs_record_id recordID)
{
    debug("Getting catalog leaf record %d of node %d", recordID, node->nodeNumber);
    
    if (node->nodeDescriptor.kind != kBTLeafNode) return 0;
    
    const HFSBTreeNodeRecord *record    = &node->records[recordID];
    u_int16_t record_type               = hfs_get_catalog_record_type(node, recordID);
    
    *record_key     = *( (HFSPlusCatalogKey*) record->key );
    *record_value   = record->value;
    
    return record_type;
}


int8_t hfs_catalog_find_record(HFSBTreeNode *node, hfs_record_id *recordID, const HFSVolume *hfs, hfs_node_id parentFolder, const wchar_t name[256], u_int8_t nameLength)
{
    debug("Searching catalog for %d:%ls", parentFolder, name);
    
    HFSPlusCatalogKey catalogKey;
    catalogKey.parentID = parentFolder;
    catalogKey.nodeName = wcstohfsuc(name);
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
    if (getenv("DEBUG")) {
        debug("compare catalog keys (CF)");
        VisualizeHFSPlusCatalogKey(key1, "Search Key", 1);
        VisualizeHFSPlusCatalogKey(key2, "Test Key  ", 1);
    }
    if (key1->parentID == key2->parentID) {
        return FastUnicodeCompare(key1->nodeName.unicode, key1->nodeName.length, key2->nodeName.unicode, key2->nodeName.length);
    } else if (key1->parentID > key2->parentID) return 1;
    
    return -1;
}

int hfs_catalog_compare_keys_bc(const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2)
{
    if (getenv("DEBUG")) {
        debug("compare catalog keys (BC)");
        VisualizeHFSPlusCatalogKey(key1, "Search Key", 1);
        VisualizeHFSPlusCatalogKey(key2, "Test Key  ", 1);
    }
    if (key1->parentID == key2->parentID) {
        wchar_t* key1Name = hfsuctowcs(&key1->nodeName);
        wchar_t* key2Name = hfsuctowcs(&key2->nodeName);
        int result = wcscmp(key1Name, key2Name);
        free(key1Name); free(key2Name);
        return result;
    } else if (key1->parentID > key2->parentID) return 1;
    
    return -1;
}

#pragma mark Searching

wchar_t* hfsuctowcs(const HFSUniStr255* input)
{
    // Allocate the return value
    wchar_t* output = malloc(256*sizeof(wchar_t));
    
    // Get the length of the input
    int len = MIN(input->length, 255);
    
    // Iterate over the input
    for (int i = 0; i < len; i++) {
        // Copy the u16 to the wchar array
        output[i] = input->unicode[i];
    }
    
    // Terminate the output at the length
    output[len] = L'\0';
    
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
    wchar_t* wide = malloc(256);
    size_t size = mbstowcs(wide, input, 255);
    HFSUniStr255 output;
    if (size) {
        output = wcstohfsuc(wide);
    } else {
        error("Conversion error: zero size returned from mbstowcs");
    }
    free(wide);
    return output;
}

u_int16_t hfs_get_catalog_record_type (const HFSBTreeNode *node, hfs_record_id i)
{
    debug("Getting record type of %d:%d", node->nodeNumber, i);
    
    if(i >= node->nodeDescriptor.numRecords) return 0;
        
    u_int16_t type = *(u_int16_t*)node->records[i].value;
    return type;
}


#pragma mark Fetch and List

// int8_t hfs_find_catalog_record(HFSBTreeNode *node, hfs_record_id *recordID, const HFSVolume *hfs, hfs_node_id parentFolder, const wchar_t name[256], u_int8_t nameLength)

/*
 hfs_get_record_by_path
 Input: an absolute POSIX-style path
 Method:
 Split path on '/' and work from the left.
 Start with the root dir (2) for the leading '/' and find the first component's record. (eg. 2:System)
 Return if file. Return error if file and not last path component.
 Read the records after it (with the same parent ID) until the name is found.
 Return if last path component.
 Repeat until file or last path component.
 */

/*
 hfs_get_record_by_parent...
 */

/*
 hfs_list_directory
 Input: a file path
 Output: a list of node/record pairs for all files in a directory
 Method:
 Find the thread record for the directory (CNID and empty name). List all records after it with the same parent ID (traversing the node fLink pointers as needed).
 */

