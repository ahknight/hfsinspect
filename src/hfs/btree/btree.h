//
//  btree.h
//  hfsinspect
//
//  Created by Adam Knight on 11/26/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_btree_btree_h
#define hfsinspect_hfs_btree_btree_h

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "hfsinspect/cdefs.h"
#include "cache.h"

#ifndef _UUID_STRING_T
#define _UUID_STRING_T
typedef char uuid_string_t[37];
#endif /* _UUID_STRING_T */

#include "hfs/Apple/hfs_types.h"


#ifndef cmp
#define cmp(a, b) ((a) > (b) ? 1 : ((a) < (b) ? -1 : 0))
#endif

typedef BTreeKey*    BTreeKeyPtr;
typedef uint16_t     BTRecOffset;
typedef BTRecOffset* BTRecOffsetPtr;
typedef uint16_t     BTRecNum;

typedef uint16_t bt_off_t;
typedef uint16_t bt_size_t;
typedef uint32_t bt_nodeid_t;

typedef struct _BTree*     BTreePtr;
typedef struct _BTreeNode* BTreeNodePtr;
//typedef struct _BTreeNodeRecord BTreeRecord;

typedef int (*btree_key_compare_func)(const BTreeKey*, const BTreeKey*) __attribute__((nonnull));
typedef bool (*btree_walk_func)(const BTreePtr tree, const BTreeNodePtr node) __attribute__((nonnull));
typedef int (*btree_get_node_func)(BTreeNodePtr* node, const BTreePtr bTree, bt_nodeid_t nodeNum) __attribute__((nonnull));

enum {
    kBTHFSTreeType      = 0x00,
    kBTUserTreeType     = 0xF0,
    kBTReservedTreeType = 0xFF,
};


#define BTGetNode(node, tree, nodeNum) (tree)->getNode((node), (tree), (nodeNum))
#define BTFreeNode(node)               btree_free_node(node)

struct _BTree {
    BTNodeDescriptor       nodeDescriptor;      // For the header node
    BTHeaderRec            headerRecord;        // From the header node
    bt_nodeid_t            treeID;
    FILE*                  fp;                  // funopen handle
    Cache                  nodeCache;
    uint8_t*               nodeBitmap;
    size_t                 nodeBitmapSize;
    btree_key_compare_func keyCompare;          // Function used to compare the keys in this tree.
    btree_get_node_func    getNode;             // Fetch and swap a node for this tree.
    bool                   _loadingBitmap;
};

struct _BTreeNode {
    BTreePtr    bTree;                      // Parent tree
    bt_nodeid_t treeID;

    union {
        BTNodeDescriptor* nodeDescriptor;      // This node's descriptor record
        uint8_t*          data;                // Raw node data
    };

    // Cached metadata
    bt_nodeid_t nodeNumber;                 // Node number in the tree file
    size_t      nodeSize;                   // Node size in bytes (according to the tree header)
    off_t       nodeOffset;                 // Byte offset within the tree file (nodeNumber * nodeSize)

    // Data
    size_t      dataLen;                    // Length of buffer (should generally be the node size, but is always the malloc_size)
    uint32_t    recordCount;
//    char                *records __deprecated;
};

typedef struct _BTNodeRecord BTNodeRecord;
typedef BTNodeRecord*        BTNodeRecordPtr;

struct _BTNodeRecord {
    BTreePtr     bTree;
    BTreeNodePtr node;
    BTRecOffset  offset;
    uint8_t*     record;
    BTreeKeyPtr  key;
    uint8_t*     value;
    BTRecNum     recNum;
    uint16_t     recordLen;
    uint16_t     keyLen;
    uint16_t     valueLen;
};

int  btree_init          (BTreePtr btree, FILE* fp) __attribute__((nonnull));
int  btree_get_node      (BTreeNodePtr* outNode, const BTreePtr tree, bt_nodeid_t nodeNumber) __attribute__((nonnull));
void btree_free_node    (BTreeNodePtr node);
int  btree_get_record    (BTreeKeyPtr* key, uint8_t** data, const BTreeNodePtr node, BTRecNum recordID) __attribute__((nonnull(1,3)));
int  btree_walk          (const BTreePtr btree, const BTreeNodePtr node, btree_walk_func walker) __attribute__((nonnull));
int  btree_search        (BTreeNodePtr* node, BTRecNum* recordID, const BTreePtr btree, const void* searchKey) __attribute__((nonnull));
int  btree_search_node   (BTRecNum* index, const BTreePtr btree, const BTreeNodePtr node, const void* searchKey) __attribute__((nonnull));

BTRecOffset BTGetRecordOffset       (const BTreeNodePtr node, uint16_t recNum) __attribute__((nonnull));
uint8_t*    BTGetRecord             (const BTreeNodePtr node, uint16_t recNum) __attribute__((nonnull));
uint16_t    BTGetRecordKeyLength    (const BTreeNodePtr node, uint16_t recNum) __attribute__((nonnull));
int         BTGetBTNodeRecord       (BTNodeRecordPtr record, const BTreeNodePtr node, BTRecNum recNum) __attribute__((nonnull));

bool BTIsBlockUsed           (uint32_t thisAllocationBlock, void* allocationFileContents, size_t length) __attribute__((nonnull));
bool BTIsNodeUsed            (const BTreePtr bTree, bt_nodeid_t nodeNum) __attribute__((nonnull));

#endif
