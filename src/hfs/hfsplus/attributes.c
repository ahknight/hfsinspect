//
//  attributes.c
//  hfsinspect
//
//  Created by Adam Knight on 6/16/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs/hfsplus/attributes.h"

#include "hfs/hfs_io.h"
#include "hfs/btree/btree.h"
#include "misc/_endian.h"
#include <wchar.h>

int hfs_get_attribute_btree(BTreePtr *tree, const HFS *hfs)
{
    debug("Getting attribute B-Tree");
    
    static BTreePtr cachedTree;
    
    if (cachedTree == NULL) {
        debug("Creating attribute B-Tree");
        
        INIT_BUFFER(cachedTree, sizeof(struct _BTree));

        HFSFork *fork;
        if ( hfsfork_get_special(&fork, hfs, kHFSAttributesFileID) < 0 ) {
            critical("Could not create fork for Attributes BTree!");
            return -1;
        }
        
        FILE* fp = fopen_hfsfork(fork);
        btree_init(cachedTree, fp);
        cachedTree->treeID      = kHFSAttributesFileID;
        cachedTree->keyCompare  = (btree_key_compare_func)hfs_attributes_compare_keys;
        cachedTree->getNode     = hfs_attributes_get_node;
    }
    
    *tree = cachedTree;
    
    return 0;
}

int hfs_attributes_compare_keys (const HFSPlusAttrKey *key1, const HFSPlusAttrKey *key2)
{
    int result;
    
    if ( (result = cmp(key1->fileID, key2->fileID)) != 0) return result;
    {
        wchar_t name1[128], name2[128];
        memcmp(name1, key1->attrName, key1->attrNameLen); name1[key1->attrNameLen] = '\0';
        memcmp(name2, key2->attrName, key2->attrNameLen); name2[key2->attrNameLen] = '\0';
        if ( (result = wcsncmp(name1, name2, MIN(key1->attrNameLen, key2->attrNameLen))) != 0 ) return result;
    }
    if ( (result = cmp(key1->startBlock, key2->startBlock)) != 0 ) return result;
    
    return 0;
}

int hfs_attributes_get_node(BTreeNodePtr *out_node, const BTreePtr bTree, bt_nodeid_t nodeNum)
{
    assert(out_node);
    assert(bTree);
    assert(bTree->treeID == kHFSAttributesFileID);
    
    BTreeNodePtr node = NULL;
    
    if ( btree_get_node(&node, bTree, nodeNum) < 0) return -1;
    
    if (node == NULL) {
        // empty node
        return 0;
    }
    
    // Swap tree-specific structs in the records
    if (node->nodeDescriptor->kind == kBTIndexNode || node->nodeDescriptor->kind == kBTLeafNode) {
        for (int recNum = 0; recNum < node->recordCount; recNum++) {
            Bytes record = BTGetRecord(node, recNum);
            HFSPlusAttrKey *key = (HFSPlusAttrKey*)record;
            swap_HFSPlusAttrKey(key);
            
            if (node->nodeDescriptor->kind == kBTLeafNode) {
                BTRecOffset keyLen = BTGetRecordKeyLength(node, recNum);
                HFSPlusAttrRecord *attrRecord = (HFSPlusAttrRecord*)(record + keyLen);
                swap_HFSPlusAttrRecord(attrRecord);
            }
        }
    }
    
    *out_node = node;
    
    return 0;
}

