//
//  hfsinspect/types.h
//  hfsinspect
//
//  Created by Adam Knight on 4/1/14.
//  Copyright (c) 2014 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_types_h
#define hfsinspect_types_h

#include <stdint.h>
#include "hfs/Apple/hfs_types.h"

// For volume statistics
typedef struct Rank {
    uint64_t measure;
    uint32_t cnid;
    uint32_t _reserved;
} Rank;

typedef struct ForkSummary {
    uint64_t count;
    uint64_t fragmentedCount;
    uint64_t blockCount;
    uint64_t logicalSpace;
    uint64_t extentRecords;
    uint64_t extentDescriptors;
    uint64_t overflowExtentRecords;
    uint64_t overflowExtentDescriptors;
} ForkSummary;

typedef struct VolumeSummary {
    uint64_t    nodeCount;
    uint64_t    recordCount;
    uint64_t    fileCount;
    uint64_t    folderCount;
    uint64_t    aliasCount;
    uint64_t    hardLinkFileCount;
    uint64_t    hardLinkFolderCount;
    uint64_t    symbolicLinkCount;
    uint64_t    invisibleFileCount;
    uint64_t    emptyFileCount;
    uint64_t    emptyDirectoryCount;

    Rank        largestFiles[10];
    Rank        mostFragmentedFiles[10];

    ForkSummary dataFork;
    ForkSummary resourceFork;
} VolumeSummary;


#endif
