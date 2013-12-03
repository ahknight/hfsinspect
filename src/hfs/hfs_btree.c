//
//  hfs_btree.c
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs/hfs_btree.h"
#include "hfs/hfs_io.h"

int hfs_get_btree(BTreePtr *btree, const HFS *hfs, hfs_cnid_t cnid)
{
//    static BTreePtr trees[16] = {{0}};
    BTreePtr tree = NULL;
    HFSFork *fork = NULL;
    
    INIT_BUFFER(tree, sizeof(struct _BTree));
    
    if ( hfsfork_get_special(&fork, hfs, cnid) < 0 )
        goto ERR;
    
    FILE *fp = NULL;
    if ( (fp = fopen_hfsfork(fork)) == NULL )
        goto ERR;
    
    if (btree_init(tree, fp) < 0)
        goto ERR;

    tree->treeID = cnid;
    switch (cnid) {
        case kHFSAttributesFileID:
//            tree->keyCompare = (btree_key_compare_func)hfs_attributes_compare_keys;
            break;
            
        default:
            break;
    }
//    tree->keyCompare = (btree_key_compare_func)hfs_attributes_compare_keys;
    
    
    return 0;
    
ERR:
    FREE_BUFFER(tree);
    return -1;
}

