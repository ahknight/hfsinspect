//
//  operations.h
//  hfsinspect
//
//  Created by Adam Knight on 4/1/14.
//  Copyright (c) 2014 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_operations_h
#define hfsinspect_operations_h

#include <stdlib.h>
#include <string.h>             // memcpy, strXXX, etc.
#if defined(__linux__)
    #include <bsd/string.h>     // strlcpy, etc.
#endif

#include "volumes/volume.h"
#include "hfs/hfs.h"
#include "hfs/types.h"
#include "hfs/catalog.h"
#include "hfs/output_hfs.h"
#include "hfs/unicode.h"
#include "hfsinspect/types.h"
#include "logging/logging.h"    // console printing routines

extern String BTreeOptionCatalog;
extern String BTreeOptionExtents;
extern String BTreeOptionAttributes;
extern String BTreeOptionHotfiles;

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
    HFS*                hfs;
    FILE*               extract_fp;
    HFSFork*            extract_HFSFork;
    uint32_t            mode;
    bt_nodeid_t         record_parent;
    bt_nodeid_t         cnid;
    BTreeTypes          tree_type;
    BTreePtr            tree;
    bt_nodeid_t         node_id;

    char                device_path[PATH_MAX];
    char                file_path[PATH_MAX];
    char                record_filename[PATH_MAX];
    char                extract_path[PATH_MAX];
    HFSPlusCatalogFile* extract_HFSPlusCatalogFile;
} HIOptions;

void set_mode (HIOptions* options, int mode);
void clear_mode (HIOptions* options, int mode);
bool check_mode (HIOptions* options, int mode);

void die(int val, String format, ...) __attribute__(( noreturn ));

void    showFreeSpace(HIOptions* options);
void    showPathInfo(HIOptions* options);
void    showCatalogRecord(HIOptions* options, FSSpec spec, bool followThreads);
ssize_t extractFork(const HFSFork* fork, const String extractPath);
void    extractHFSPlusCatalogFile(const HFS* hfs, const HFSPlusCatalogFile* file, const String extractPath);

VolumeSummary generateVolumeSummary(HIOptions* options);
void          generateForkSummary(HIOptions* options, ForkSummary* forkSummary, const HFSPlusCatalogFile* file, const HFSPlusForkData* fork, hfs_forktype_t type);

#endif
