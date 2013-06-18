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
#include <stdio.h>

HFSBTree hfs_get_attribute_btree(const HFSVolume *hfs)
{
    debug("Getting attribute B-Tree");
    
    static HFSBTree tree;
    if (tree.fork.cnid == 0) {
        debug("Creating attribute B-Tree");
        
        HFSFork fork = hfsfork_make(hfs, hfs->vh.catalogFile, HFSDataForkType, kHFSAttributesFileID);
        
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
