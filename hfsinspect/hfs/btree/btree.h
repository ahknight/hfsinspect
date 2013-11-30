//
//  btree.h
//  hfsinspect
//
//  Created by Adam Knight on 11/26/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_btree_btree_h
#define hfsinspect_hfs_btree_btree_h

#include "cache.h"
#include <hfs/hfs_format.h>
#include <stdint.h>

typedef uint8_t     Byte;
typedef Byte        *Bytes;
typedef BTreeKey    *BTreeKeyPtr;
typedef uint16_t    BTRecOffset;
typedef BTRecOffset *BTRecOffsetPtr;
typedef uint16_t    BTRecNum;

typedef uint16_t    bt_off_t;
typedef uint16_t    bt_size_t;
typedef uint32_t    bt_nodeid_t;

typedef struct _BTree *BTreePtr;
typedef struct _BTreeNode *BTreeNodePtr;
typedef struct _BTreeNodeRecord BTreeRecord __deprecated;

typedef int(*btree_key_compare_func)(const BTreeKey *, const BTreeKey *);
typedef bool(*btree_walk_func)(const BTreePtr tree, const BTreeNodePtr node);
typedef int(*btree_get_node_func)(BTreeNodePtr *node, const BTreePtr bTree, bt_nodeid_t nodeNum);

#define BTGetNode(node, tree, nodeNum) (tree)->getNode((node), (tree), (nodeNum))

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
    char                *records __deprecated;
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

int btree_init          (BTreePtr btree, FILE *fp);
int btree_get_node      (BTreeNodePtr *outNode, const BTreePtr tree, bt_nodeid_t nodeNumber);
void btree_free_node    (BTreeNodePtr node);
int btree_get_record    (BTreeKeyPtr * key, Bytes * data, const BTreeNodePtr node, BTRecNum recordID);
int btree_walk          (const BTreePtr btree, const BTreeNodePtr node, btree_walk_func walker);
int btree_search        (BTreeNodePtr *node, BTRecNum *recordID, const BTreePtr btree, const void * searchKey);
int btree_search_node   (BTRecNum *index, const BTreePtr btree, const BTreeNodePtr node, const void * searchKey);

BTRecOffset BTGetRecordOffset       (const BTreeNodePtr node, uint8_t recNum);
Bytes       BTGetRecord             (const BTreeNodePtr node, uint8_t recNum);
uint16_t    BTGetRecordKeyLength    (const BTreeNodePtr node, uint8_t recNum);
int         BTGetBTNodeRecord       (BTNodeRecordPtr record, const BTreeNodePtr node, BTRecNum recNum);

bool        BTIsBlockUsed           (uint32_t thisAllocationBlock, Bytes allocationFileContents);
bool        BTIsNodeUsed            (const BTreePtr bTree, bt_nodeid_t nodeNum);

#endif
