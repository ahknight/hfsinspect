//
//  hotfiles.c
//  hfsinspect
//
//  Created by Adam Knight on 12/3/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs/hfsplus/hotfiles.h"
#include "hfs/catalog.h"
#include "hfs/hfs_io.h"

int hfs_get_hotfiles_btree(BTreePtr *tree, const HFS *hfs)
{
    debug("Getting hotfiles B-Tree");
    
    static BTreePtr cachedTree;
    
    if (cachedTree == NULL) {
        debug("Creating hotfiles B-Tree");
        
        BTreeNodePtr node = NULL;
        BTRecNum recordID = 0;
        bt_nodeid_t parentfolder = kHFSRootFolderID;
        HFSUniStr255 name = wcstohfsuc(L".hotfiles.btree");
        int found = hfs_catalog_find_record(&node, &recordID, hfs, parentfolder, name);
        if (found != 1)
            return -1;
        
        BTreeKeyPtr recordKey = NULL;
        Bytes recordValue = NULL;
        btree_get_record(&recordKey, &recordValue, node, recordID);
        
        HFSPlusCatalogRecord* record = (HFSPlusCatalogRecord*)recordValue;
        HFSFork *fork = NULL;
        if ( hfsfork_make(&fork, hfs, record->catalogFile.dataFork, 0x00, record->catalogFile.fileID) < 0 )
            return -1;
        
        INIT_BUFFER(cachedTree, sizeof(struct _BTree));
        FILE* fp = fopen_hfsfork(fork);
        if (fp == NULL) return -1;
        int result = btree_init(cachedTree, fp);
        if (result < 1) {
            error("Error initializing hotfiles btree.");
            return -1;
        }
        cachedTree->treeID = record->catalogFile.fileID;
        cachedTree->getNode = hfs_hotfiles_get_node;
    }
    
    // Copy the cached tree out.
    // Note this copies a reference to the same extent list in the HFSFork struct so NEVER free that fork.
    *tree = cachedTree;
    
    return 0;
}

int hfs_hotfiles_get_node(BTreeNodePtr *out_node, const BTreePtr bTree, bt_nodeid_t nodeNum)
{
    assert(out_node);
    assert(bTree);
    assert(bTree->treeID > 16);
    
    BTreeNodePtr node = NULL;
    
    if ( btree_get_node(&node, bTree, nodeNum) < 0) return -1;
    
    if (node == NULL) {
        // empty node
        return 0;
    }
    
    // Swap catalog-specific structs in the records
//    if (node->nodeDescriptor->kind == kBTIndexNode || node->nodeDescriptor->kind == kBTLeafNode) {
//        for (int recNum = 0; recNum < node->recordCount; recNum++) {
//            Bytes record = NULL;
//            
//            // Get the raw record
//            record = BTGetRecord(node, recNum);
//        }
//    }
    
    *out_node = node;
    
    return 0;
}
