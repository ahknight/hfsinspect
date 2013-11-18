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
#include "output_hfs.h"

int hfs_get_extents_btree(BTree* tree, const HFS *hfs)
{
    debug("Get extents B-Tree");
    
    static BTree* cachedTree;
    
    if (cachedTree == NULL) {
        debug("Creating extents B-Tree");
        
        INIT_BUFFER(cachedTree, sizeof(BTree));
        
        HFSFork *fork;
        if ( hfsfork_get_special(&fork, hfs, kHFSExtentsFileID) < 0 ) {
            critical("Could not create fork for Extents BTree!");
            return -1;
        }
        
        btree_init(cachedTree, fork);
        
        cachedTree->keyCompare = (hfs_compare_keys)hfs_extents_compare_keys;
    }
    
    // Copy the cached tree out.
    // Note this copies a reference to the same extent list in the HFSFork struct so NEVER free that fork.
    *tree = *cachedTree;
    
    return 0;
}

int hfs_extents_find_record(HFSPlusExtentRecord *record, hfs_block_t *record_start_block, const HFSFork *fork, size_t startBlock)
{
    debug("Finding extents record for CNID %d with start block %d", fork->cnid, startBlock);
    
    int result;
    
    HFS *hfs = fork->hfs;
    bt_nodeid_t fileID = fork->cnid;
    hfs_forktype_t forkType = fork->forkType;
    
    BTree extentsTree;
    hfs_get_extents_btree(&extentsTree, hfs);
    BTreeNode* node = NULL;
    bt_recordid_t index = 0;
    
    if (extentsTree.headerRecord.rootNode == 0) {
        //Empty tree; no extents
        result = false;
        goto RETURN;
    }
    
    HFSPlusExtentKey extentKey;
    extentKey.keyLength = kHFSPlusExtentKeyMaximumLength;
    extentKey.fileID = fileID;
    extentKey.forkType = forkType;
    extentKey.startBlock = (hfs_block_t)startBlock;
    
    result = btree_search_tree(&node, &index, &extentsTree, &extentKey);
    
    /*
     The result returned is mostly irrelevant as this function can be used to find the record that contains the
     start block and not just that matches it.  To that end, verify the key is valid in that context and
     produce a return value based on that.
     */
    
    if (node->dataLen == 0) {
        debug("No extent record returned for %d:%d:%d", extentKey.fileID, extentKey.forkType, extentKey.startBlock);
        if (result == true) {
            // Told we had a record and did not, that's an error.
            result = -1;
            goto RETURN;
            
        } else {
            // Not promised a result, so this is fine.
            result = false;
            goto RETURN;
        }
    }
    
    // Buffer has some data, so validate it.
    const BTreeRecord *returnedRecord    = &node->records[index];
    HFSPlusExtentKey* returnedExtentKey         = (HFSPlusExtentKey*)returnedRecord->key;
    HFSPlusExtentRecord* returnedExtentRecord   = (HFSPlusExtentRecord*)returnedRecord->value;
    
    if (returnedExtentKey->fileID != fileID || returnedExtentKey->forkType != forkType || returnedExtentKey->startBlock > startBlock) {
        warning("Returned key is not valid for the request (requested %d:%d:%d; received %d:%d:%d)", fileID, forkType, startBlock, returnedExtentKey->fileID, returnedExtentKey->forkType, returnedExtentKey->startBlock);
        result = false;
        goto RETURN;
    }
    
    // At ths point the record looks like it should be valid, so pass it along.
    memcpy(*record, returnedExtentRecord, returnedRecord->valueLength);
    *record_start_block = returnedExtentKey->startBlock;
    
RETURN:
    btree_free_node(node);
    return result;
}

int hfs_extents_compare_keys(const HFSPlusExtentKey *key1, const HFSPlusExtentKey *key2)
{
//    if (DEBUG) {
//        debug("compare extent keys");
//        VisualizeHFSPlusExtentKey(key1, "Search Key", 1);
//        VisualizeHFSPlusExtentKey(key2, "Test Key  ", 1);
//    }
    
    int result;
    if ( (result = cmp(key1->fileID, key2->fileID)) != 0) return result;
    if ( (result = cmp(key1->forkType, key2->forkType)) != 0) return result;
    if ( (result = cmp(key1->startBlock, key2->startBlock)) != 0) return result;
    return 0;
}

bool hfs_extents_get_extentlist_for_fork(ExtentList* list, const HFSFork* fork)
{
    unsigned blocks = 0;
    extentlist_add_record(list, fork->forkData.extents);
    for (int i = 0; i < kHFSPlusExtentDensity; i++) blocks += fork->forkData.extents[i].blockCount;
    
    while (blocks < fork->totalBlocks) {
        debug("Fetching more extents");
        HFSPlusExtentRecord record;
        hfs_block_t startBlock = 0;
        
        int found = hfs_extents_find_record(&record, &startBlock, fork, blocks);
        
        if (found < 0) {
            error("Error while searching extent B-Tree for additional extents.");
            return false;
        }
        
        if (found == 0) break;
        
        if (startBlock > blocks)
            critical("Bad extent.");
        
        if ( extentlist_find(list, startBlock, NULL, NULL) == true )
            critical("We already have this record.");
        
        extentlist_add_record(list, record);
        
        FOR_UNTIL(i, kHFSPlusExtentDensity)
            blocks += record[i].blockCount;
        
        if (record[7].blockCount == 0)
            break;
    }
    
    return true;
}
