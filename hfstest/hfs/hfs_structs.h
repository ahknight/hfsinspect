//
//  hfs_structs.h
//  hfstest
//
//  Created by Adam Knight on 5/6/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfstest_hfs_structs_h
#define hfstest_hfs_structs_h

#import "buffer.h"

struct HFSVolume {
    int                 fd;                 // File descriptor
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
};
typedef struct HFSBTree HFSBTree;

struct HFSBTreeNode {
    Buffer              buffer;             // Raw data buffer
    HFSBTree            bTree;              // Parent tree
    BTNodeDescriptor    nodeDescriptor;     // This node's descriptor record
    size_t              blockSize;          // Node/buffer size in bytes
    u_int32_t           nodeNumber;         // Node number in the tree file
    u_int32_t           nodeOffset;         // Block offset within the tree file
};
typedef struct HFSBTreeNode HFSBTreeNode;

#endif
