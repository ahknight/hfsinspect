//
//  hfs_attribute_ops.c
//  hfsinspect
//
//  Created by Adam Knight on 6/16/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs_attribute_ops.h"
#include "hfs_io.h"
#include "hfs_btree.h"
#include <wchar.h>

int hfs_get_attribute_btree(BTree* tree, const HFS *hfs)
{
    debug("Getting attribute B-Tree");
    
    static BTree* cachedTree;
    
    if (cachedTree == NULL) {
        debug("Creating attribute B-Tree");
        
        INIT_BUFFER(cachedTree, sizeof(BTree));

        HFSFork *fork;
        if ( hfsfork_get_special(&fork, hfs, kHFSAttributesFileID) < 0 ) {
            critical("Could not create fork for Attributes BTree!");
            return -1;
        }
        
        btree_init(cachedTree, fork);
        cachedTree->keyCompare = (hfs_compare_keys)hfs_attributes_compare_keys;
    }
    
    // Copy the cached tree out.
    // Note this copies a reference to the same extent list in the HFSFork struct so NEVER free that fork.
    *tree = *cachedTree;
    
    return 0;
}

int hfs_attributes_compare_keys (const HFSPlusAttrKey *key1, const HFSPlusAttrKey *key2)
{
    int result;
    
    if ( (result = cmp(key1->fileID, key2->fileID)) != 0) return result;
    {
        wchar_t name1[128], name2[128];
        memset(name1, key1->attrName, key1->attrNameLen); name1[key1->attrNameLen] = '\0';
        memset(name2, key2->attrName, key2->attrNameLen); name2[key2->attrNameLen] = '\0';
        if ( (result = wcsncmp(name1, name2, MIN(key1->attrNameLen, key2->attrNameLen))) != 0 ) return result;
    }
    if ( (result = cmp(key1->startBlock, key2->startBlock)) != 0 ) return result;
    
    return 0;
}
