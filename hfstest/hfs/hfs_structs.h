//
//  hfs_structs.h
//  hfstest
//
//  Created by Adam Knight on 5/6/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <hfs/hfs_format.h>
#include "buffer.h"

#ifndef hfstest_hfs_structs_h
#define hfstest_hfs_structs_h

typedef u_int8_t    hfs_fork_type;
typedef u_int32_t   hfs_block;
typedef u_int64_t   hfs_size;
typedef u_int32_t   hfs_node_id;
typedef u_int32_t   hfs_record_id;
typedef u_int16_t   hfs_record_offset;

typedef int(*hfs_compare_keys)(const void*, const void*);

struct HFSVolume {
    int                 fd;                 // File descriptor
    FILE                *fp;                // File pointer
    HFSPlusVolumeHeader vh;                 // Volume header
};
typedef struct HFSVolume HFSVolume;

struct HFSFork {
    HFSVolume           hfs;                // File system descriptor
    HFSPlusForkData     forkData;           // Contains the initial extents
    u_int8_t            forkType;           // 0x00: data; 0xFF: resource
    u_int32_t           cnid;               // For extents overflow lookups
};
typedef struct HFSFork HFSFork;

struct HFSBTree {
    HFSFork             fork;               // For data access
    BTNodeDescriptor    nodeDescriptor;     // For the header node
    BTHeaderRec         headerRecord;       // From the header node
    hfs_compare_keys    keyCompare;         // Function used to compare the keys in this tree.
};
typedef struct HFSBTree HFSBTree;

struct HFSBTreeNodeRecord {
    hfs_record_id       recordID;
    hfs_record_offset   offset;
    hfs_record_offset   length;
    void*               record;
    hfs_record_offset   keyLength;
    void*               key;
    hfs_record_offset   valueLength;
    void*               value;
};
typedef struct HFSBTreeNodeRecord HFSBTreeNodeRecord;

struct HFSBTreeNode {
    struct Buffer       buffer;             // Raw data buffer
    HFSBTree            bTree;              // Parent tree
    BTNodeDescriptor    nodeDescriptor;     // This node's descriptor record
    size_t              nodeSize;          // Node/buffer size in bytes
    u_int32_t           nodeNumber;         // Node number in the tree file
    off_t               nodeOffset;         // Block offset within the tree file
    u_int16_t           recordCount;
    HFSBTreeNodeRecord  records[512];
};
typedef struct HFSBTreeNode HFSBTreeNode;

#endif
