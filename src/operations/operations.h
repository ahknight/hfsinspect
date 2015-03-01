//
//  operations.h
//  hfsinspect
//
//  Created by Adam Knight on 4/1/14.
//  Copyright (c) 2014 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_operations_h
#define hfsinspect_operations_h


#include "volumes/volume.h"
#include "hfs/hfs.h"
#include "hfs/types.h"
#include "hfs/catalog.h"
#include "hfs/output_hfs.h"
#include "hfs/unicode.h"
#include "logging/logging.h"    // console printing routines

extern char* BTreeOptionCatalog;
extern char* BTreeOptionExtents;
extern char* BTreeOptionAttributes;
extern char* BTreeOptionHotfiles;

typedef enum BTreeTypes {
    BTreeTypeCatalog = 0,
    BTreeTypeExtents,
    BTreeTypeAttributes,
    BTreeTypeHotfiles
} BTreeTypes;

enum HIModes {
    HIModeShowVolumeInfo = 0,
    HIModeShowJournalInfo,
    HIModeShowSummary,
    HIModeShowBTreeInfo,
    HIModeShowBTreeNode,
    HIModeShowCatalogRecord,
    HIModeShowCNID,
    HIModeShowPathInfo,
    HIModeExtractFile,
    HIModeListFolder,
    HIModeShowDiskInfo,
    HIModeYankFS,
    HIModeFreeSpace,
};

// Configuration context
typedef struct HIOptions {
    HFSPlus*            hfs;
    BTreePtr            tree;
    FILE*               extract_fp;
    HFSPlusFork*        extract_HFSPlusFork;
    HFSPlusCatalogFile* extract_HFSPlusCatalogFile;

    uint32_t            mode;
    bt_nodeid_t         record_parent;
    bt_nodeid_t         cnid;
    bt_nodeid_t         node_id;
    BTreeTypes          tree_type;

    char                device_path[PATH_MAX];
    char                file_path[PATH_MAX];
    char                record_filename[PATH_MAX];
    char                extract_path[PATH_MAX];
} HIOptions;

void set_mode (HIOptions* options, int mode);
void clear_mode (HIOptions* options, int mode);
bool check_mode (HIOptions* options, int mode);

void die(int val, char* format, ...) __attribute__(( noreturn ));

void    showFreeSpace(HIOptions* options);
void    showPathInfo(HIOptions* options);
void    showCatalogRecord(HIOptions* options, FSSpec spec, bool followThreads);
ssize_t extractFork(const HFSPlusFork* fork, const char* extractPath);
void    extractHFSPlusCatalogFile(const HFSPlus* hfs, const HFSPlusCatalogFile* file, const char* extractPath);


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


VolumeSummary generateVolumeSummary(HIOptions* options);
void          generateForkSummary(HIOptions* options, ForkSummary* forkSummary, const HFSPlusCatalogFile* file, const HFSPlusForkData* fork, hfs_forktype_t type);
void          PrintVolumeSummary             (out_ctx* ctx, const VolumeSummary* summary) _NONNULL;
void          PrintForkSummary               (out_ctx* ctx, const ForkSummary* summary) _NONNULL;

#endif
