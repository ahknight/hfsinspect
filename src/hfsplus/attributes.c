//
//  attributes.c
//  hfsinspect
//
//  Created by Adam Knight on 6/16/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <wchar.h>

#include "hfsplus/hfsplus.h"

#include "hfs/hfs_io.h"
#include "hfs/btree/btree.h"
#include "volumes/utilities.h" // commonly-used utility functions
#include "logging/logging.h"   // console printing routines


int hfsplus_get_attribute_btree(BTreePtr* tree, const HFSPlus* hfs)
{
    static BTreePtr cachedTree = NULL;

    debug("Getting attribute B-Tree");

    if (cachedTree == NULL) {
        HFSPlusFork* fork = NULL;
        FILE*        fp   = NULL;

        debug("Creating attribute B-Tree");

        SALLOC(cachedTree, sizeof(struct _BTree));

        if ( hfsplus_get_special_fork(&fork, hfs, kHFSAttributesFileID) < 0 ) {
            critical("Could not create fork for Attributes B-Tree!");
            return -1;
        }

        fp                     = fopen_hfsfork(fork);
        btree_init(cachedTree, fp);
        cachedTree->treeID     = kHFSAttributesFileID;
        cachedTree->keyCompare = (btree_key_compare_func)hfsplus_attributes_compare_keys;
        cachedTree->getNode    = hfsplus_attributes_get_node;
    }

    *tree = cachedTree;

    return 0;
}

int hfsplus_attributes_compare_keys (const HFSPlusAttrKey* key1, const HFSPlusAttrKey* key2)
{
    int     result     = 0;
	
	HFSUniStr255 key1UniStr = {key1->attrNameLen, {0}};
	HFSUniStr255 key2UniStr = {key2->attrNameLen, {0}};
	
	memcpy(&key1UniStr->attrName, key1->attrName, kHFSMaxAttrNameLen);
	memcpy(&key2UniStr->attrName, key2->attrName, kHFSMaxAttrNameLen);

    hfs_str key1Name = {0};
    hfs_str key2Name = {0};

    hfsuc_to_str(&key1Name, key1UniStr);
    hfsuc_to_str(&key2Name, key2UniStr);

    trace("BC compare: key1 (%p) (%u, %u, '%s'), key2 (%p) (%u, %u, '%s')",
          key1, key1->fileID, key1->attrNameLen, key1Name,
          key2, key2->fileID, key2->attrNameLen, key2Name);

    if ( (result = cmp(key1->fileID, key2->fileID)) != 0)
        return result;

    unsigned len = MIN(key1->attrNameLen, key2->attrNameLen);
    for (unsigned i = 0; i < len; i++) {
        if ((result = cmp(key1->attrName[i], key2->attrName[i])) != 0) {
            trace("* Character difference at position %u", i);
            break;
        }
    }

    if (result == 0) {
        // The shared prefix sorted the same, so the shorter one wins.
        result = cmp(key1->attrNameLen, key2->attrNameLen);
        if (result != 0) trace("* Shorter wins.");
    }

    return 0;
}

int hfsplus_attributes_get_node(BTreeNodePtr* out_node, const BTreePtr bTree, bt_nodeid_t nodeNum)
{
    BTreeNodePtr node = NULL;

    assert(out_node);
    assert(bTree);
    assert(bTree->treeID == kHFSAttributesFileID);

    if ( btree_get_node(&node, bTree, nodeNum) < 0)
        return -1;

    // Handle empty nodes (error?)
    if (node == NULL) {
        return 0;
    }

    // Swap tree-specific structs in the records
    if ((node->nodeDescriptor->kind == kBTIndexNode) || (node->nodeDescriptor->kind == kBTLeafNode)) {
        for (unsigned recNum = 0; recNum < node->recordCount; recNum++) {
            void*           record = BTGetRecord(node, recNum);
            HFSPlusAttrKey* key    = (HFSPlusAttrKey*)record;

            swap_HFSPlusAttrKey(key);

            if (node->nodeDescriptor->kind == kBTLeafNode) {
                BTRecOffset keyLen     = BTGetRecordKeyLength(node, recNum);
                void*       attrRecord = ((char*)record + keyLen);

                swap_HFSPlusAttrRecord((HFSPlusAttrRecord*)attrRecord);
            }
        }
    }

    *out_node = node;

    return 0;
}

void HFSPlusPrintFileAttributes(uint32_t fileID, HFSPlus* hfs)
{
    BTreeNodePtr      	node     = NULL;
    BTRecNum           	recordID = 0;
	HFSPlusAttrKey		key = {0};
	
	key.keyLength = 14;
	key.fileID = fileID;
	key.attrNameLen = 0;

    if (hfsplus_get_attribute_btree(&tree, hfs) < 0)
        return -1;

    int found = btree_search(&node, &recordID, tree, &key);
    if ((found != 1) || (node->dataLen == 0)) {
        warning("No thread record for %d found.", cnid);
        return -1;
    }

    debug("Found thread record %d:%d", node->nodeNumber, recordID);

    BTreeKeyPtr           recordKey    = NULL;
    void*                 recordValue  = NULL;
    btree_get_record(&recordKey, &recordValue, node, recordID);

    HFSPlusCatalogThread* threadRecord = (HFSPlusCatalogThread*)recordValue;
}