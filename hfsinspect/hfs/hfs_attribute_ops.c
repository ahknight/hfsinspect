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

HFSBTree hfs_get_attribute_btree(const HFSVolume *hfs)
{
    debug("Getting attribute B-Tree");
    
    static HFSBTree tree;
    if (tree.fork.cnid == 0) {
        debug("Creating attribute B-Tree");
        
        HFSFork fork = hfsfork_get_special(hfs, kHFSAttributesFileID);
        
        hfs_btree_init(&tree, &fork);
        
//        if (tree.headerRecord.keyCompareType == 0xCF) {
//            // Case Folding (normal; case-insensitive)
//            tree.keyCompare = (hfs_compare_keys)hfs_catalog_compare_keys_cf;
//            
//        } else if (tree.headerRecord.keyCompareType == 0xBC) {
//            // Binary Compare (case-sensitive)
//            tree.keyCompare = (hfs_compare_keys)hfs_catalog_compare_keys_bc;
//            
//        }
    }
    
    return tree;
}

int cmp(int64_t a, int64_t b) {
    return ( ((a) > (b)) ? 1 : ( ((a) < (b)) ? -1 : 0 ) );
}

int hfs_attributes_compare_keys (const HFSPlusAttrKey *key1, const HFSPlusAttrKey *key2)
{
    int result;
    result = cmp(key1->fileID, key2->fileID);
    if (result != 0) return result;
    
    wchar_t name1[128], name2[128];
    memset(name1, key1->attrName, key1->attrNameLen); name1[key1->attrNameLen] = '\0';
    memset(name2, key2->attrName, key2->attrNameLen); name2[key2->attrNameLen] = '\0';
    
    int sort = wcsncmp(name1, name2, MIN(key1->attrNameLen, key2->attrNameLen));
    
    if (sort != 0) return sort;
    
    return cmp(key1->startBlock, key2->startBlock);
}
