//
//  btree.h
//  hfsinspect
//
//  Created by Adam Knight on 11/26/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_btree_btree_h
#define hfsinspect_hfs_btree_btree_h

#include "misc/cache.h"
#include "hfs/Apple/hfs_format.h"
#include "hfs/Apple/hfs_macos_defs.h"
#include <stdint.h>

typedef BytePtr     Bytes;
typedef BTreeKey    *BTreeKeyPtr;
typedef uint16_t    BTRecOffset;
typedef BTRecOffset *BTRecOffsetPtr;
typedef uint16_t    BTRecNum;

typedef uint16_t    bt_off_t;
typedef uint16_t    bt_size_t;
typedef uint32_t    bt_nodeid_t;

typedef struct _BTree *BTreePtr;
typedef struct _BTreeNode *BTreeNodePtr;
//typedef struct _BTreeNodeRecord BTreeRecord;

typedef int(*btree_key_compare_func)(const BTreeKey *, const BTreeKey *) _NONNULL;
typedef bool(*btree_walk_func)(const BTreePtr tree, const BTreeNodePtr node) _NONNULL;
typedef int(*btree_get_node_func)(BTreeNodePtr *node, const BTreePtr bTree, bt_nodeid_t nodeNum) _NONNULL;

enum {
    kBTHFSTreeType = 0,
    kBTUserTreeType = 128,
    kBTReservedTreeType = 255,
};


#define BTGetNode(node, tree, nodeNum) (tree)->getNode((node), (tree), (nodeNum))
#define BTFreeNode(node) btree_free_node(node)

struct _BTree {
    BTNodeDescriptor        nodeDescriptor;     // For the header node
    BTHeaderRec             headerRecord;       // From the header node
    bt_nodeid_t             treeID;
    FILE                    *fp;                // funopen handle
    Cache                   nodeCache;
    btree_key_compare_func  keyCompare;         // Function used to compare the keys in this tree.
    btree_get_node_func     getNode;            // Fetch and swap a node for this tree.
};

struct _BTreeNode {
    BTreePtr            bTree;              // Parent tree
    bt_nodeid_t         treeID;
    
    union {
        BTNodeDescriptor    *nodeDescriptor;     // This node's descriptor record
        Bytes               data;              // Raw node data
    };
    
    // Cached metadata
    bt_nodeid_t         nodeNumber;         // Node number in the tree file
    size_t              nodeSize;           // Node size in bytes (according to the tree header)
    off_t               nodeOffset;         // Byte offset within the tree file (nodeNumber * nodeSize)
    
    // Data
    size_t              dataLen;            // Length of buffer (should generally be the node size, but is always the malloc_size)
    uint32_t            recordCount;
//    char                *records __deprecated;
};

typedef struct _BTNodeRecord BTNodeRecord;
typedef BTNodeRecord *BTNodeRecordPtr;

struct _BTNodeRecord {
    BTreePtr        bTree;
    BTreeNodePtr    node;
    BTRecOffset     offset;
    Bytes           record;
    BTreeKeyPtr     key;
    Bytes           value;
    BTRecNum        recNum;
    uint16_t        recordLen;
    uint16_t        keyLen;
    uint16_t        valueLen;
};

int btree_init          (BTreePtr btree, FILE *fp) _NONNULL;
int btree_get_node      (BTreeNodePtr *outNode, const BTreePtr tree, bt_nodeid_t nodeNumber) _NONNULL;
void btree_free_node    (BTreeNodePtr node);
int btree_get_record    (BTreeKeyPtr * key, Bytes * data, const BTreeNodePtr node, BTRecNum recordID) NONNULL(1,3);
int btree_walk          (const BTreePtr btree, const BTreeNodePtr node, btree_walk_func walker) _NONNULL;
int btree_search        (BTreeNodePtr *node, BTRecNum *recordID, const BTreePtr btree, const void * searchKey) _NONNULL;
int btree_search_node   (BTRecNum *index, const BTreePtr btree, const BTreeNodePtr node, const void * searchKey) _NONNULL;

BTRecOffset BTGetRecordOffset       (const BTreeNodePtr node, uint8_t recNum) _NONNULL;
Bytes       BTGetRecord             (const BTreeNodePtr node, uint8_t recNum) _NONNULL;
uint16_t    BTGetRecordKeyLength    (const BTreeNodePtr node, uint8_t recNum) _NONNULL;
int         BTGetBTNodeRecord       (BTNodeRecordPtr record, const BTreeNodePtr node, BTRecNum recNum) _NONNULL;

bool        BTIsBlockUsed           (uint32_t thisAllocationBlock, Bytes allocationFileContents) _NONNULL;
bool        BTIsNodeUsed            (const BTreePtr bTree, bt_nodeid_t nodeNum) _NONNULL;

#endif
