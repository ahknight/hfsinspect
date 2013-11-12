//
//  hfs_structs.h
//  hfsinspect
//
//  Created by Adam Knight on 5/6/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_structs_h
#define hfsinspect_hfs_structs_h

#include "volume.h"
#include "buffer.h"
#include "hfs_macos_defs.h"

#include <hfs/hfs_format.h>     //duh

typedef uint8_t    hfs_fork_type;
typedef uint32_t   hfs_block;
typedef uint64_t   hfs_size;
typedef uint32_t   hfs_node_id;
typedef uint32_t   hfs_record_id;
typedef uint16_t   hfs_record_offset;

typedef int(*hfs_compare_keys)(const void*, const void*);

typedef struct HFSVolume            HFSVolume;
typedef struct HFSFork              HFSFork;
typedef struct HFSBTree             HFSBTree;
typedef struct HFSBTreeNode         HFSBTreeNode;
typedef struct HFSBTreeNodeRecord   HFSBTreeNodeRecord;

struct HFSVolume {
    Volume*             vol;                // Volume containing the filesystem
    HFSPlusVolumeHeader vh;                 // Volume header
    off_t               offset;             // Partition offset, if known/needed (bytes)
    size_t              length;             // Partition length, if known/needed (bytes)
    size_t              block_size;         // Allocation block size. (bytes)
    size_t              block_count;        // Number of blocks. (blocks of block_size size)
};

struct HFSFork {
    HFSVolume           hfs;                // File system descriptor
    HFSPlusForkData     forkData;           // Contains the initial extents
    hfs_fork_type       forkType;           // 0x00: data; 0xFF: resource
    hfs_node_id         cnid;               // For extents overflow lookups
    hfs_block           totalBlocks;
    hfs_size            logicalSize;
    struct _ExtentList  *extents;           // All known extents
};

struct HFSBTree {
    HFSFork             fork;               // For data access
    BTNodeDescriptor    nodeDescriptor;     // For the header node
    BTHeaderRec         headerRecord;       // From the header node
    hfs_compare_keys    keyCompare;         // Function used to compare the keys in this tree.
};

struct HFSBTreeNodeRecord {
    hfs_node_id         treeCNID;
    hfs_node_id         nodeID;
    HFSBTreeNode*       node;
    hfs_record_id       recordID;
    hfs_record_offset   offset;
    hfs_record_offset   length;
    char*               record;
    hfs_record_offset   keyLength;
    char*               key;
    hfs_record_offset   valueLength;
    char*               value;
};

struct HFSBTreeNode {
    struct Buffer       buffer;             // Raw data buffer
    HFSBTree            bTree;              // Parent tree
    BTNodeDescriptor    nodeDescriptor;     // This node's descriptor record
    size_t              nodeSize;           // Node/buffer size in bytes
    hfs_node_id         nodeNumber;         // Node number in the tree file
    off_t               nodeOffset;         // Block offset within the tree file
    hfs_record_id       recordCount;
    HFSBTreeNodeRecord  records[512];
};


// For volume statistics
typedef struct Rank {
    uint64_t   measure;
    hfs_node_id cnid;
    
} Rank;

typedef struct ForkSummary {
    uint64_t   count;
    uint64_t   fragmentedCount;
    uint64_t   blockCount;
    uint64_t   logicalSpace;
    uint64_t   extentRecords;
    uint64_t   extentDescriptors;
    uint64_t   overflowExtentRecords;
    uint64_t   overflowExtentDescriptors;
    
} ForkSummary;

typedef struct VolumeSummary {
    uint64_t   nodeCount;
    uint64_t   recordCount;
    uint64_t   fileCount;
    uint64_t   folderCount;
    uint64_t   aliasCount;
    uint64_t   hardLinkFileCount;
    uint64_t   hardLinkFolderCount;
    uint64_t   symbolicLinkCount;
    uint64_t   invisibleFileCount;
    uint64_t   emptyFileCount;
    uint64_t   emptyDirectoryCount;
    
    Rank        largestFiles[10];
    Rank        mostFragmentedFiles[10];
    
    ForkSummary dataFork;
    ForkSummary resourceFork;
    
} VolumeSummary;


//Patching up a section of the volume header
union HFSPlusVolumeFinderInfo {
    uint8_t 	finderInfo[32];
    struct {
        uint32_t   bootDirID;
        uint32_t   bootParentID;
        uint32_t   openWindowDirID;
        uint32_t   os9DirID;
        uint32_t   reserved;
        uint32_t   osXDirID;
        uint64_t   volID;
    };
};
typedef union HFSPlusVolumeFinderInfo HFSPlusVolumeFinderInfo;

// Makes catalog records a bit easier.
union HFSPlusCatalogRecord {
    int16_t                 record_type;
    HFSPlusCatalogFile      catalogFile;
    HFSPlusCatalogFolder    catalogFolder;
    HFSPlusCatalogThread    catalogThread;
};
typedef union HFSPlusCatalogRecord HFSPlusCatalogRecord;

#endif
