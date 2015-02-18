//
//  attributes.c
//  hfsinspect
//
//  Created by Adam Knight on 6/16/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfsplus/hfsplus.h"

#include "hfs/hfs_io.h"
#include "hfs/btree/btree.h"
#include "volumes/utilities.h" // commonly-used utility functions
#include "logging/logging.h"   // console printing routines


int hfs_get_attribute_btree(BTreePtr* tree, const HFS* hfs)
{
    static BTreePtr cachedTree = NULL;

    debug("Getting attribute B-Tree");

    if (cachedTree == NULL) {
        HFSFork* fork = NULL;
        FILE*    fp   = NULL;

        debug("Creating attribute B-Tree");

        SALLOC(cachedTree, sizeof(struct _BTree));

        if ( hfsfork_get_special(&fork, hfs, kHFSAttributesFileID) < 0 ) {
            critical("Could not create fork for Attributes B-Tree!");
            return -1;
        }

        fp                     = fopen_hfsfork(fork);
        btree_init(cachedTree, fp);
        cachedTree->treeID     = kHFSAttributesFileID;
        cachedTree->keyCompare = (btree_key_compare_func)hfs_attributes_compare_keys;
        cachedTree->getNode    = hfs_attributes_get_node;
    }

    *tree = cachedTree;

    return 0;
}

int hfs_attributes_compare_keys (const HFSPlusAttrKey* key1, const HFSPlusAttrKey* key2)
{
    int     result     = 0;
    wchar_t name1[128] = L"";
    wchar_t name2[128] = L"";

    if ( (result = cmp(key1->fileID, key2->fileID)) != 0)
        return result;

    memcpy(name1, key1->attrName, key1->attrNameLen); name1[key1->attrNameLen] = '\0';
    memcpy(name2, key2->attrName, key2->attrNameLen); name2[key2->attrNameLen] = '\0';

    if ( (result = wcsncmp(name1, name2, MIN(key1->attrNameLen, key2->attrNameLen))) != 0 )
        return result;

    if ( (result = cmp(key1->startBlock, key2->startBlock)) != 0 )
        return result;

    return 0;
}

int hfs_attributes_get_node(BTreeNodePtr* out_node, const BTreePtr bTree, bt_nodeid_t nodeNum)
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

