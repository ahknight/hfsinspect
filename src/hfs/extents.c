//
//  hfs_extent_ops.c
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <errno.h>              // errno/perror
#include <string.h>             // memcpy, strXXX, etc.
#include <assert.h>             // assert()

#include "hfs/extents.h"

#include "volumes/output.h"
#include "volumes/utilities.h" // commonly-used utility functions
#include "hfs/hfs_io.h"
#include "hfs/output_hfs.h"
#include "logging/logging.h"   // console printing routines


int hfs_get_extents_btree(BTreePtr* tree, const HFS* hfs)
{
    assert(tree);
    assert(hfs);

    debug("Get extents B-Tree");

    static BTreePtr cachedTree;

    if (cachedTree == NULL) {
        debug("Creating extents B-Tree");

        ALLOC(cachedTree, sizeof(struct _BTree));

        HFSFork* fork;
        if ( hfsfork_get_special(&fork, hfs, kHFSExtentsFileID) < 0 ) {
            critical("Could not create fork for Extents B-Tree!");
        }
        FILE*    fp = fopen_hfsfork(fork);
        if (fp == NULL)
            return -1;
        btree_init(cachedTree, fp);

        cachedTree->treeID     = kHFSExtentsFileID;
        cachedTree->keyCompare = (btree_key_compare_func)hfs_extents_compare_keys;
        cachedTree->getNode    = hfs_extents_get_node;
    }

    *tree = cachedTree;

    return 0;
}

int hfs_extents_get_node(BTreeNodePtr* out_node, const BTreePtr bTree, bt_nodeid_t nodeNum)
{
    BTreeNodePtr node = NULL;
    out_ctx      ctx  = OCMake(0, 2, "extents");

    assert(out_node);
    assert(bTree);
    assert(bTree->treeID == kHFSExtentsFileID);

    if ( btree_get_node(&node, bTree, nodeNum) < 0) return -1;

    if (node == NULL) {
        // empty node
        return 0;
    }

    // Swap tree-specific structs in the records
    if ((node->nodeDescriptor->kind == kBTIndexNode) || (node->nodeDescriptor->kind == kBTLeafNode)) {
        for (int recNum = 0; recNum < node->recordCount; recNum++) {
            BTNodeRecord      record = {0};
            BTGetBTNodeRecord(&record, node, recNum);
            HFSPlusExtentKey* key    = (HFSPlusExtentKey*)record.key;
            swap_HFSPlusExtentKey(key);
            if (key->keyLength != kHFSPlusExtentKeyMaximumLength) {
                warning("Invalid extent key! (%#08x != %#08x @ %p)", key->keyLength, kHFSPlusExtentKeyMaximumLength, key);
                VisualizeHFSPlusExtentKey(&ctx, key, "Bad Key", 0);
                continue;
            }

            if (node->nodeDescriptor->kind == kBTLeafNode) {
                swap_HFSPlusExtentRecord(((HFSPlusExtentDescriptor*)record.value));
//                PrintHFSPlusExtentRecord(&ctx, ((const HFSPlusExtentRecord*)record.value));
            }
        }
    }

    *out_node = node;

    return 0;
}

int hfs_extents_find_record(HFSPlusExtentRecord* record, hfs_block_t* record_start_block, const HFSFork* fork, size_t startBlock)
{
    debug("Finding extents record for CNID %d with start block %zu", fork->cnid, startBlock);

    int            result;

    HFS*           hfs         = fork->hfs;
    bt_nodeid_t    fileID      = fork->cnid;
    hfs_forktype_t forkType    = fork->forkType;

    BTreePtr       extentsTree = NULL;
    if ( hfs_get_extents_btree(&extentsTree, hfs) < 0)
        return -1;
    BTreeNodePtr   node        = NULL;
    BTRecNum       index       = 0;

    if (extentsTree->headerRecord.rootNode == 0) {
        //Empty tree; no extents
        result = false;
        goto RETURN;
    }

    HFSPlusExtentKey searchKey;
    searchKey.keyLength  = kHFSPlusExtentKeyMaximumLength;
    searchKey.fileID     = fileID;
    searchKey.forkType   = forkType;
    searchKey.startBlock = (hfs_block_t)startBlock;

    result               = btree_search(&node, &index, extentsTree, &searchKey);

    /*
       The result returned is mostly irrelevant as this function can be used to find the record that contains the
       start block and not just that matches it.  To that end, verify the key is valid in that context and
       produce a return value based on that.
     */

    if (node->dataLen == 0) {
        debug("No extent record returned for %d:%d:%d", searchKey.fileID, searchKey.forkType, searchKey.startBlock);
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
    BTreeKeyPtr          recordKey      = NULL;
    uint8_t*             recordValue    = NULL;
    btree_get_record(&recordKey, &recordValue, node, index);

    HFSPlusExtentKey*    returnedKey    = (HFSPlusExtentKey*)recordKey;
    HFSPlusExtentRecord* returnedRecord = (HFSPlusExtentRecord*)recordValue;

    if ((returnedKey->fileID != searchKey.fileID) || (returnedKey->forkType != searchKey.forkType) || (returnedKey->startBlock > searchKey.startBlock)) {
        warning("Returned key is not valid for the request (requested %d:%d:%zu; received %d:%d:%d)", fileID, forkType, startBlock, returnedKey->fileID, returnedKey->forkType, returnedKey->startBlock);
        result = false;
        goto RETURN;
    }

    // At ths point the record looks like it should be valid, so pass it along.
    if (record != NULL) memcpy(*record, returnedRecord, sizeof(HFSPlusExtentRecord));
    *record_start_block = returnedKey->startBlock;

RETURN:
    btree_free_node(node);
    return result;
}

int hfs_extents_compare_keys(const HFSPlusExtentKey* key1, const HFSPlusExtentKey* key2)
{
//    if (DEBUG) {
//        debug("compare extent keys");
//        out_ctx ctx = OCMake(0, 2, "extents");
//        VisualizeHFSPlusExtentKey(&ctx, key1, "Search Key", 1);
//        VisualizeHFSPlusExtentKey(&ctx, key2, "Test Key  ", 1);
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
        hfs_block_t         startBlock = 0;

        int                 found      = hfs_extents_find_record(&record, &startBlock, fork, blocks);

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

