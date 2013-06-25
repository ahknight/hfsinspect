//
//  hfs_structs.h
//  hfsinspect
//
//  Created by Adam Knight on 5/6/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <sys/param.h>
#include <hfs/hfs_format.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include "buffer.h"
#include "hfs_extentlist.h"

#ifndef hfsinspect_hfs_structs_h
#define hfsinspect_hfs_structs_h

typedef u_int8_t    hfs_fork_type;
typedef u_int32_t   hfs_block;
typedef u_int64_t   hfs_size;
typedef u_int32_t   hfs_node_id;
typedef u_int32_t   hfs_record_id;
typedef u_int16_t   hfs_record_offset;

typedef int(*hfs_compare_keys)(const void*, const void*);

typedef struct HFSVolume            HFSVolume;
typedef struct HFSFork              HFSFork;
typedef struct HFSBTree             HFSBTree;
typedef struct HFSBTreeNode         HFSBTreeNode;
typedef struct HFSBTreeNodeRecord   HFSBTreeNodeRecord;

struct HFSVolume {
    int                 fd;                 // File descriptor
    FILE                *fp;                // File pointer
    char                device[PATH_MAX];   // Device path
    struct statfs       stat_fs;            // statfs record for the device path
    struct stat         stat;               // stat record for the device path
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
    u_int64_t   measure;
    hfs_node_id cnid;
    
} Rank;

typedef struct ForkSummary {
    u_int64_t   count;
    u_int64_t   fragmentedCount;
    u_int64_t   blockCount;
    u_int64_t   logicalSpace;
    u_int64_t   extentRecords;
    u_int64_t   extentDescriptors;
    u_int64_t   overflowExtentRecords;
    u_int64_t   overflowExtentDescriptors;
    
} ForkSummary;

typedef struct VolumeSummary {
    u_int64_t   nodeCount;
    u_int64_t   recordCount;
    u_int64_t   fileCount;
    u_int64_t   folderCount;
    u_int64_t   aliasCount;
    u_int64_t   hardLinkFileCount;
    u_int64_t   hardLinkFolderCount;
    u_int64_t   symbolicLinkCount;
    u_int64_t   invisibleFileCount;
    u_int64_t   emptyFileCount;
    u_int64_t   emptyDirectoryCount;
    
    Rank        largestFiles[10];
    Rank        mostFragmentedFiles[10];
    
    ForkSummary dataFork;
    ForkSummary resourceFork;
    
} VolumeSummary;


//Patching up a section of the volume header
union HFSPlusVolumeFinderInfo {
    u_int8_t 	finderInfo[32];
    struct {
        u_int32_t   bootDirID;
        u_int32_t   bootParentID;
        u_int32_t   openWindowDirID;
        u_int32_t   os9DirID;
        u_int32_t   reserved;
        u_int32_t   osXDirID;
        u_int64_t   volID;
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
