//
//  hfs_structs.h
//  hfsinspect
//
//  Created by Adam Knight on 5/6/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_structs_h
#define hfsinspect_hfs_structs_h

#include "cache.h"
#include "volume.h"
#include "buffer.h"
#include "hfs_macos_defs.h"

#include <hfs/hfs_format.h>     //duh

typedef uint8_t     hfs_forktype_t;
typedef uint32_t    hfs_block_t;
typedef uint64_t    hfs_size_t;
typedef uint32_t    bt_nodeid_t;
typedef uint32_t    hfs_cnid_t;
typedef uint32_t    bt_recordid_t;
typedef uint16_t    bt_off_t;
typedef uint16_t    bt_size_t;

typedef wchar_t     hfs_wc_str[256]; // Wide char version of HFSUniStr255

typedef int(*hfs_compare_keys)(const void*, const void*);

typedef struct HFS          HFS;
typedef struct HFSFork      HFSFork;
typedef struct BTree        BTree;
typedef struct BTreeNode    BTreeNode;
typedef struct BTreeRecord  BTreeRecord;

struct HFS {
    Volume*             vol;                // Volume containing the filesystem
    HFSPlusVolumeHeader vh;                 // Volume header
    off_t               offset;             // Partition offset, if known/needed (bytes)
    size_t              length;             // Partition length, if known/needed (bytes)
    size_t              block_size;         // Allocation block size. (bytes)
    size_t              block_count;        // Number of blocks. (blocks of block_size size)
};

struct HFSFork {
    HFS                 *hfs;               // File system descriptor
    HFSPlusForkData     forkData;           // Contains the initial extents
    hfs_forktype_t      forkType;           // 0x00: data; 0xFF: resource
    bt_nodeid_t         cnid;               // For extents overflow lookups
    hfs_block_t         totalBlocks;
    hfs_size_t          logicalSize;
    struct _ExtentList  *extents;           // All known extents
};

struct BTree {
    hfs_cnid_t          treeID;
    BTNodeDescriptor    nodeDescriptor;     // For the header node
    BTHeaderRec         headerRecord;       // From the header node
    FILE                *fp;                // funopen handle
    Cache               nodeCache;
    hfs_compare_keys    keyCompare;         // Function used to compare the keys in this tree.
};

struct BTreeRecord {
    bt_nodeid_t         treeCNID;
    bt_nodeid_t         nodeID;
    BTreeNode           *node;
    bt_recordid_t       recordID;
    bt_off_t            offset;
    bt_size_t           length;
    char                *record;
    bt_size_t           keyLength;
    BTreeKey            *key;
    bt_size_t           valueLength;
    char                *value;
};

struct BTreeNode {
    BTree               bTree;              // Parent tree
    hfs_cnid_t          treeID;
    
    // Cached metadata
    BTNodeDescriptor    nodeDescriptor;     // This node's descriptor record
    bt_nodeid_t         nodeNumber;         // Node number in the tree file
    size_t              nodeSize;           // Node size in bytes (according to the tree header)
    off_t               nodeOffset;         // Byte offset within the tree file (nodeNumber * nodeSize)
    
    // Data
    char                *data;              // Raw node data
    size_t              dataLen;            // Length of buffer (should generally be the node size, but is always the malloc_size)
    uint32_t            recordCount;
    BTreeRecord         records[512];
};


// For volume statistics
typedef struct Rank {
    uint64_t            measure;
    hfs_cnid_t          cnid;
} Rank;

typedef struct ForkSummary {
    uint64_t            count;
    uint64_t            fragmentedCount;
    uint64_t            blockCount;
    uint64_t            logicalSpace;
    uint64_t            extentRecords;
    uint64_t            extentDescriptors;
    uint64_t            overflowExtentRecords;
    uint64_t            overflowExtentDescriptors;
} ForkSummary;

typedef struct VolumeSummary {
    uint64_t            nodeCount;
    uint64_t            recordCount;
    uint64_t            fileCount;
    uint64_t            folderCount;
    uint64_t            aliasCount;
    uint64_t            hardLinkFileCount;
    uint64_t            hardLinkFolderCount;
    uint64_t            symbolicLinkCount;
    uint64_t            invisibleFileCount;
    uint64_t            emptyFileCount;
    uint64_t            emptyDirectoryCount;
    
    Rank                largestFiles[10];
    Rank                mostFragmentedFiles[10];
    
    ForkSummary         dataFork;
    ForkSummary         resourceFork;
} VolumeSummary;


//Patching up a section of the volume header
union HFSPlusVolumeFinderInfo {
    uint8_t             finderInfo[32];
    struct {
        uint32_t        bootDirID;
        uint32_t        bootParentID;
        uint32_t        openWindowDirID;
        uint32_t        os9DirID;
        uint32_t        reserved;
        uint32_t        osXDirID;
        uint64_t        volID;
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
