//
//  hfs/types.h
//  hfsinspect
//
//  Created by Adam Knight on 5/6/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_structs_h
#define hfsinspect_hfs_structs_h

#ifndef _UUID_STRING_T
#define _UUID_STRING_T
typedef char uuid_string_t[37];
#endif /* _UUID_STRING_T */

#include "volumes/volume.h"
#include "hfs/Apple/hfs_types.h"

#include "hfs/btree/btree.h"

typedef uint8_t  hfs_forktype_t;
typedef uint32_t hfs_block_t;
typedef uint32_t hfs_cnid_t;
typedef uint64_t hfs_size_t;

typedef wchar_t hfs_wc_str[256];     // Wide char version of HFSUniStr255

typedef int (* hfs_compare_keys)(const void*, const void*);

typedef struct HFS     HFS;
typedef struct HFSFork HFSFork;

struct HFS {
    Volume*             vol;                // Volume containing the filesystem
    HFSPlusVolumeHeader vh;                 // Volume header
    off_t               offset;             // Partition offset, if known/needed (bytes)
    size_t              length;             // Partition length, if known/needed (bytes)
    size_t              block_size;         // Allocation block size. (bytes)
    size_t              block_count;        // Number of blocks. (blocks of block_size size)
};

struct HFSFork {
    HFS*                hfs;                // File system descriptor
    hfs_size_t          logicalSize;
    hfs_block_t         totalBlocks;
    bt_nodeid_t         cnid;               // For extents overflow lookups
    hfs_forktype_t      forkType;           // 0x00: data; 0xFF: resource
    uint8_t             _reserved1[3];
    HFSPlusForkData     forkData;           // Contains the initial extents
    uint8_t             _reserved2[4];
    struct _ExtentList* extents;            // All known extents
};

//Patching up a section of the volume header
union HFSPlusVolumeFinderInfo {
    uint8_t finderInfo[32];
    struct {
        uint32_t bootDirID;
        uint32_t bootParentID;
        uint32_t openWindowDirID;
        uint32_t os9DirID;
        uint32_t reserved;
        uint32_t osXDirID;
        uint64_t volID;
    };
};
typedef union HFSPlusVolumeFinderInfo HFSPlusVolumeFinderInfo;

// Makes catalog records a bit easier.
union HFSPlusCatalogRecord {
    int16_t              record_type;
    HFSPlusCatalogFile   catalogFile;
    HFSPlusCatalogFolder catalogFolder;
    HFSPlusCatalogThread catalogThread;
};
typedef union HFSPlusCatalogRecord HFSPlusCatalogRecord;

#endif
