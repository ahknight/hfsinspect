//
//  hfs_extents.c
//  hfstest
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs_extent_ops.h"
#include "hfs_io.h"
#include "hfs_btree.h"
#include "hfs_pstruct.h"

#include <stdio.h>

HFSBTree hfs_get_extents_btree(const HFSVolume *hfs)
{
    HFSFork fork = make_hfsfork(hfs, hfs->vh.extentsFile, HFSDataForkType, kHFSExtentsFileID);
    
    HFSBTree tree;
    hfs_btree_init(&tree, &fork);
    
    tree.keyCompare = (hfs_compare_keys)hfs_extents_compare_keys;
    
    return tree;
}

int8_t hfs_extents_find_record(HFSPlusExtentRecord *record, size_t *record_start_block, const HFSFork *fork, size_t startBlock)
{
    HFSVolume hfs = fork->hfs;
    hfs_node_id fileID = fork->cnid;
    hfs_fork_type forkType = fork->forkType;
    
    HFSPlusExtentKey extentKey = {};
    extentKey.keyLength = kHFSPlusExtentKeyMaximumLength;
    extentKey.fileID = fileID;
    extentKey.forkType = forkType;
    extentKey.startBlock = (hfs_block)startBlock;
    
    HFSBTree extentsTree = hfs_get_extents_btree(&hfs);
    HFSBTreeNode node = {};
    hfs_record_id index = 0;
    
    bool result = hfs_btree_search_tree(&node, &index, &extentsTree, &extentKey);
    
    if (node.buffer.size) {
        const HFSBTreeNodeRecord *returnedRecord = &node.records[index];
        HFSPlusExtentKey* returnedExtentKey = (HFSPlusExtentKey*)returnedRecord->key;
        HFSPlusExtentRecord* returnedExtentRecord = (HFSPlusExtentRecord*)returnedRecord->value ;
        
        if (returnedExtentKey->fileID != fileID || returnedExtentKey->forkType != forkType || returnedExtentKey->startBlock > startBlock) {
            return -1;
        } else {
            memcpy(*record, returnedExtentRecord, returnedRecord->valueLength);
            *record_start_block = returnedExtentKey->startBlock;
        }
    }
    
    return result;
}

int hfs_extents_compare_keys(const HFSPlusExtentKey *key1, const HFSPlusExtentKey *key2)
{
    if (getenv("DEBUG")) {
        debug("compare extent keys");
        VisualizeHFSPlusExtentKey(key1, "Search Key", 1);
        VisualizeHFSPlusExtentKey(key2, "Test Key  ", 1);
    }
    if (key1->fileID == key2->fileID) {
        if (key1->forkType == key2->forkType) {
            if (key1->startBlock == key2->startBlock) {
                return 0; // equality
            } else if (key1->startBlock > key2->startBlock) return 1;
        } else if (key1->forkType > key2->forkType) return 1;
    } else if (key1->fileID > key2->fileID) return 1;
    
    return -1;
}

