//
//  hfs_extent_ops.c
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs_extent_ops.h"

#include "hfs_io.h"
#include "hfs_btree.h"

HFSBTree hfs_get_extents_btree(const HFSVolume *hfs)
{
    debug("Get extents B-Tree");
    
    static HFSBTree tree;
    if (tree.fork.cnid == 0) {
        debug("Creating extents B-Tree");
        HFSFork fork = hfsfork_get_special(hfs, kHFSExtentsFileID);
        
        hfs_btree_init(&tree, &fork);
        
        tree.keyCompare = (hfs_compare_keys)hfs_extents_compare_keys;
    }
    
    return tree;
}

int8_t hfs_extents_find_record(HFSPlusExtentRecord *record, hfs_block *record_start_block, const HFSFork *fork, size_t startBlock)
{
    debug("Finding extents record for CNID %d with start block %d", fork->cnid, startBlock);
    
    HFSVolume hfs = fork->hfs;
    hfs_node_id fileID = fork->cnid;
    hfs_fork_type forkType = fork->forkType;
    
    HFSBTree extentsTree = hfs_get_extents_btree(&hfs);
    HFSBTreeNode node;
    hfs_record_id index = 0;
    
    if (extentsTree.headerRecord.rootNode == 0) {
        //Empty tree; no extents
        return false;
    }
    
    HFSPlusExtentKey extentKey;
    extentKey.keyLength = kHFSPlusExtentKeyMaximumLength;
    extentKey.fileID = fileID;
    extentKey.forkType = forkType;
    extentKey.startBlock = (hfs_block)startBlock;
    
    bool result = hfs_btree_search_tree(&node, &index, &extentsTree, &extentKey);
    
    /*
     The result returned is mostly irrelevant as this function can be used to find the record that contains the
     start block and not just that matches it.  To that end, verify the key is valid in that context and
     produce a return value based on that.
     */
    
    if (node.buffer.size == 0) {
        debug("No extent record returned for %d:%d:%d", extentKey.fileID, extentKey.forkType, extentKey.startBlock);
        if (result == true)
            return -1;      // Told we had a record and did not, that's an error.
        else
            return false;   // Not promised a result, so this is fine.
    }
    
    // Buffer has some data, so validate it.
    const HFSBTreeNodeRecord *returnedRecord    = &node.records[index];
    HFSPlusExtentKey* returnedExtentKey         = (HFSPlusExtentKey*)returnedRecord->key;
    HFSPlusExtentRecord* returnedExtentRecord   = (HFSPlusExtentRecord*)returnedRecord->value;
    
    if (returnedExtentKey->fileID != fileID || returnedExtentKey->forkType != forkType || returnedExtentKey->startBlock > startBlock) {
        debug("Returned key is not valid for the request (requested %d:%d:%d; received %d:%d:%d)", fileID, forkType, startBlock, returnedExtentKey->fileID, returnedExtentKey->forkType, returnedExtentKey->startBlock);
        return false;
    }
    
    // At ths point the record looks like it should be valid, so pass it along.
    memcpy(*record, returnedExtentRecord, returnedRecord->valueLength);
    *record_start_block = returnedExtentKey->startBlock;
    
    return true;
}

int hfs_extents_compare_keys(const HFSPlusExtentKey *key1, const HFSPlusExtentKey *key2)
{
//    if (getenv("DEBUG")) {
//        debug("compare extent keys");
//        VisualizeHFSPlusExtentKey(key1, "Search Key", 1);
//        VisualizeHFSPlusExtentKey(key2, "Test Key  ", 1);
//    }
    if (key1->fileID == key2->fileID) {
        if (key1->forkType == key2->forkType) {
            if (key1->startBlock == key2->startBlock) {
                return 0; // equality
            } else if (key1->startBlock > key2->startBlock) return 1;
        } else if (key1->forkType > key2->forkType) return 1;
    } else if (key1->fileID > key2->fileID) return 1;
    
    return -1;
}

bool hfs_extents_get_extentlist_for_fork(ExtentList* list, const HFSFork* fork)
{
    unsigned blocks = 0;
    extentlist_add_record(list, fork->forkData.extents);
    for (int i = 0; i < kHFSPlusExtentDensity; i++) blocks += fork->forkData.extents[i].blockCount;
    
    while (blocks < fork->totalBlocks) {
        debug("Fetching more extents");
        HFSPlusExtentRecord record;
        hfs_block startBlock = 0;
        int found = hfs_extents_find_record(&record, &startBlock, fork, blocks);
        if (found < 0) {
            error("Error while searching extent B-Tree for additional extents.");
            return false;
        } else if (found > 0) {
            if (startBlock > blocks) {
                critical("Bad extent.");
            }
            size_t offset=0, length=0;
            if (extentlist_find(list, startBlock, &offset, &length)) {
                critical("We already have this record.");
            }
            extentlist_add_record(list, record);
            for (int i = 0; i < kHFSPlusExtentDensity; i++) blocks += record[i].blockCount;
            if (record[7].blockCount == 0) break;
        } else {
            break;
        }
    }
    
    return true;
}