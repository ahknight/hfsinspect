//
//  main.c
//  hfsinspect
//
//  Created by Adam Knight on 5/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <getopt.h>         //getopt_long
#include <libgen.h>         //basename
#include <locale.h>         //setlocale
#include <sys/param.h>      //MIN/MAX

#if defined(__APPLE__)
#include <sys/mount.h>      //statfs
#endif

#include <sys/types.h>      //stat
#include <sys/stat.h>       //stat
#include <string.h>
#include <stdarg.h>
#include <xlocale.h>

#include "misc/output.h"
#include "misc/stringtools.h"
#include "hfs/hfs.h"
#include "hfs/output_hfs.h"
#include "volumes/volumes.h"

#pragma mark - Function Declarations

void    die                         (int val, String format, ...) __attribute__(( noreturn ));

void    set_mode                    (int mode);
void    clear_mode                  (int mode);
bool    check_mode                  (int mode);

void    usage                       (int status) __attribute__(( noreturn ));
void    show_version                (void) __attribute__(( noreturn ));

void    showPathInfo                (void);
void    showFreeSpace               (void);
void    showCatalogRecord           (FSSpec spec, bool followThreads);

int     compare_ranked_files        (const void * a, const void * b) _NONNULL;
VolumeSummary generateVolumeSummary (HFS* hfs) _NONNULL;
void    generateForkSummary         (ForkSummary* forkSummary, const HFSPlusCatalogFile* file, const HFSPlusForkData* fork, hfs_forktype_t type) _NONNULL;

ssize_t extractFork                 (const HFSFork* fork, const String extractPath) _NONNULL;
void    extractHFSPlusCatalogFile   (const HFS *hfs, const HFSPlusCatalogFile* file, const String extractPath) _NONNULL;

String  deviceAtPath                (String path) _NONNULL;
bool    resolveDeviceAndPath        (String path_in, String device_out, String path_out) _NONNULL;


#pragma mark - Variable Definitions

char            PROGRAM_NAME[PATH_MAX];

const String    BTreeOptionCatalog      = "catalog";
const String    BTreeOptionExtents      = "extents";
const String    BTreeOptionAttributes   = "attributes";
const String    BTreeOptionHotfiles     = "hotfiles";

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
static struct HIOptions {
    uint32_t   mode;
    
    char        device_path[PATH_MAX];
    char        file_path[PATH_MAX];
    
    bt_nodeid_t record_parent;
    char        record_filename[PATH_MAX];
    
    bt_nodeid_t cnid;
    
    char                extract_path[PATH_MAX];
    FILE*               extract_fp;
    HFSPlusCatalogFile  extract_HFSPlusCatalogFile;
    HFSFork             *extract_HFSFork;
    
    HFS   hfs;
    BTreeTypes  tree_type;
    BTreePtr    tree;
    bt_nodeid_t node_id;
} HIOptions;

#pragma mark - Function Definitions

#define EMPTY_STRING(x) ((x) == NULL || strlen((x)) == 0)

void set_mode(int mode)     { HIOptions.mode |= (1 << mode); }
void clear_mode(int mode)   { HIOptions.mode &= ~(1 << mode); }
bool check_mode(int mode)   { return (HIOptions.mode & (1 << mode)); }

void usage(int status)
{
    /* hfsdebug-lite args remaining...
     -0,        --freespace     display all free extents on the volume
     -e,        --examples      display some usage examples
     -f,        --fragmentation display all fragmented files on the volume
     -H,        --hotfiles      display the hottest files on the volume; requires
                                    the -t (--top=TOP) option for the number to list
     -l TYPE,   --list=TYPE     specify an HFS+ B-Tree's leaf nodes' type, where
                                     TYPE is one of "file", "folder", "filethread", or
                                     "folderthread" for the Catalog B-Tree; one of
                                     "hfcfile" or "hfcthread" for the Hot Files B-Tree
                                     B-Tree; or "extents" for the Extents B-Tree
                                     You can use the type "any" if you wish to display
                                     all node types. Currently, "any" is the only
                                     type supported for the Attributes B-Tree
     -m,        --mountdata     display a mounted volume's in-memory data
     -S,        --summary_rsrc  calculate and display volume usage summary
     -t TOP,    --top=TOP       specify the number of most fragmented files
                                     to display (when used with the --fragmentation
                                     option), or the number of largest files to display
                                     (when used with the --summary option)
     -x FILTERDYLIB, --filter=FILTERDYLIB
                                 run the filter implemented in the dynamic library
                                     whose path is FILTERDYLIB. Alternatively,
                                     FILTERDYLIB can be 'builtin:<name>', where <name>
                                     specifies one of the built-in filters: atime,
                                     crtime, dirhardlink, hardlink, mtime, and sxid
     -X FILTERARGS, --filter_args=FILTERARGS
                                 pass this argument string to the filter callback
     */
    String help = "[-hvjs] [-d file] [-V volumepath] [ [-b btree] [-n nodeid] ] [-p path] [-P absolutepath] [-F parent:name] [-o file] [--version] [path]\n"
    "\n"
    "SOURCES: \n"
    "hfsinspect will use the root filesystem by default, or the filesystem containing a target file in some cases. If you wish to\n"
    "specify a specific device or volume, you can with the following options:\n"
    "\n"
    "    -d DEV,     --device DEV    Path to device or file containing a bare HFS+ filesystem (no partition map or HFS wrapper) \n"
    "    -V VOLUME   --volume VOLUME Use the path to a mounted disk or any file on the disk to use a mounted volume. \n"
    "\n"
    "INFO: \n"
    "    By default, hfsinspect will just show you the volume header and quit.  Use the following options to get more specific data.\n"
    "\n"
    "    -h,         --help          Show help and quit. \n"
    "    -v,         --version       Show version information and quit. \n"
    "    -b NAME,    --btree NAME    Specify which HFS+ B-Tree to work with. Supported options: attributes, catalog, extents, or hotfiles. \n"
    "    -n ID,      --node ID       Dump an HFS+ B-Tree node by ID (must specify tree with -b). \n"
    "    -r,         --volumeheader  Dump the volume header. \n"
    "    -j,         --journal       Dump the volume's journal info block structure. \n"
    "    -c CNID,    --cnid CNID     Lookup and display a record by its catalog node ID. \n"
    "    -l,         --list          If the specified FSOB is a folder, list the contents. \n"
    "    -D,         --disk-info     Show any available information about the disk, including partitions and volume headers.\n"
    "    -0,         --freespace     Show a summary of the used/free space and extent count based on the allocation file.\n"
    "\n"
    "OUTPUT: \n"
    "    You can optionally have hfsinspect dump any fork it finds as the result of an operation. This includes B-Trees or file forks.\n"
    "    Use a command like \"-b catalog -o catalog.dump\" to extract the catalog file from the boot drive, for instance.\n"
    "\n"
    "    -o PATH,    --output PATH   Use with -b or -p to dump a raw data fork (when used with -b, will dump the HFS+ tree file). \n"
    "\n"
    "ENVIRONMENT: \n"
"    Set NOCOLOR to hide ANSI colors (TODO: check terminal for support, etc.). \n"
"    Set DEBUG to be overwhelmed with useless data. \n"
"\n";
    fprintf(stderr, "usage: %s %s", PROGRAM_NAME, help);
    exit(status);
}

void show_version()
{
    debug("Version requested.");
    print("%s %s\n", PROGRAM_NAME, "1.0");
    exit(0);
}

void showFreeSpace(void)
{
    HFSFork *fork = NULL;
    if ( hfsfork_get_special(&fork, &HIOptions.hfs, kHFSAllocationFileID) < 0 )
        die(1, "Couldn't get a reference to the volume allocation file.");
    
    char *data = calloc(fork->logicalSize, 1);
    {
        size_t block_size = 128*1024, offset = 0;
        ssize_t bytes = 0;
        void *block = calloc(block_size, 1);
        while ( (bytes = hfs_read_fork_range(block, fork, block_size, offset)) > 0 ) {
            memcpy(data+offset, block, bytes);
            offset += bytes;
            if (offset >= fork->logicalSize) break;
        }
        free(block);
        
        if (offset == 0) {
            free(data);
            
            // We didn't read anything.
            if (bytes < 0) {
                // Error
                die(errno, "error reading allocation file");
            }
            
            return;
        }
    }
    
    size_t total_used = 0;
    size_t total_free = 0;
    size_t total_extents = 0;
    
    struct extent {
        uint8_t     used;
        size_t      start;
        size_t      length;
    };
    
    // The first allocation block is used by the VH.
    struct extent currentExtent = {1,0,1};
    
    for (size_t i = 0; i < HIOptions.hfs.vh.totalBlocks; i++) {
        bool used = BTIsBlockUsed(i, data, fork->logicalSize);
        if (used == currentExtent.used && (i != (HIOptions.hfs.vh.totalBlocks - 1))) {
            currentExtent.length++;
            continue;
        }
        
        total_extents += 1;
        
        if (currentExtent.used)
            total_used += currentExtent.length;
        else
            total_free += currentExtent.length;
        
        currentExtent.used = used;
        currentExtent.start = i;
        currentExtent.length = 1;
    }
    
    BeginSection("Allocation File Statistics");
    PrintAttribute("Extents", "%zu", total_extents);
    _PrintHFSBlocks("Used Blocks", total_used);
    _PrintHFSBlocks("Free Blocks", total_free);
    _PrintHFSBlocks("Total Blocks", total_used + total_free);
    EndSection();
    
    free(data);
    hfsfork_free(fork);
}

void showPathInfo(void)
{
    debug("Finding records for path: %s", HIOptions.file_path);
    
    if (strlen(HIOptions.file_path) == 0) {
        error("Path is required.");
        exit(1);
    }
    
    FSSpec                  spec = {0};
    HFSPlusCatalogRecord    catalogRecord = {0};
    
    if ( HFSPlusGetCatalogInfoByPath(&spec, &catalogRecord, HIOptions.file_path, &HIOptions.hfs) < 0) {
        error("Path not found: %s", HIOptions.file_path);
        return;
    }
    
    showCatalogRecord(spec, false);
    
    if (catalogRecord.record_type == kHFSFileRecord) {
        HIOptions.extract_HFSPlusCatalogFile = catalogRecord.catalogFile;
    }
}

void showCatalogRecord(FSSpec spec, bool followThreads)
{
    hfs_wc_str filename_str = {0};
    hfsuctowcs(filename_str, &spec.name);
    debug("Finding catalog record for %d:%ls", spec.parentID, filename_str);
    
    HFSPlusCatalogRecord catalogRecord = {0};
    if ( HFSPlusGetCatalogRecordByFSSpec(&catalogRecord, spec) < 0 ) {
        error("FSSpec {%u, '%ls'} not found.", spec.parentID, filename_str);
        return;
    }
    
    int type = catalogRecord.record_type;
    
    if (type == kHFSPlusFileThreadRecord || type == kHFSPlusFolderThreadRecord) {
        if (followThreads) {
            debug("Following thread for %d:%ls", spec.parentID, filename_str);
            FSSpec s = {
                .hfs = spec.hfs,
                .parentID = catalogRecord.catalogThread.parentID,
                .name = catalogRecord.catalogThread.nodeName
            };
            showCatalogRecord(s, followThreads);
            return;
            
        } else {
            if (type == kHFSPlusFolderThreadRecord && check_mode(HIModeListFolder)) {
                PrintFolderListing(spec.parentID);
            } else {
                BTreeNodePtr node = NULL; BTRecNum recordID = 0;
                hfs_catalog_find_record(&node, &recordID, spec);
                PrintNodeRecord(node, recordID);
            }
        }
        
    } else if (type == kHFSPlusFileRecord) {
        // Check for hard link
        if (catalogRecord.catalogFile.userInfo.fdCreator == kHardLinkFileType && catalogRecord.catalogFile.userInfo.fdType == kHFSPlusCreator) {
            print("Record is a hard link.");
        }
        
        // Check for soft link
        if (catalogRecord.catalogFile.userInfo.fdCreator == kSymLinkCreator && catalogRecord.catalogFile.userInfo.fdType == kSymLinkFileType) {
            print("Record is a symbolic link.");
        }
        
        // Display record
        BTreeNodePtr node = NULL; BTRecNum recordID = 0;
        hfs_catalog_find_record(&node, &recordID, spec);
        PrintNodeRecord(node, recordID);
        
        // Set extract file
        HIOptions.extract_HFSPlusCatalogFile = catalogRecord.catalogFile;
        
    } else if (type == kHFSPlusFolderRecord) {
        if (check_mode(HIModeListFolder)) {
            PrintFolderListing(catalogRecord.catalogFolder.folderID);
        } else {
            BTreeNodePtr node = NULL; BTRecNum recordID = 0;
            hfs_catalog_find_record(&node, &recordID, spec);
            PrintNodeRecord(node, recordID);
        }
        
    } else {
        error("Unknown record type: %d", type);
    }
}

int compare_ranked_files(const void * a, const void * b)
{
    const Rank *A = (Rank*)a;
    const Rank *B = (Rank*)b;
    
    int result = cmp(A->measure, B->measure);
    
    return result;
}

VolumeSummary generateVolumeSummary(HFS* hfs)
{
    /*
     Walk the leaf catalog nodes and gather various stats about the volume as a whole.
     */
    
    VolumeSummary summary = {0};
    
    BTreePtr catalog = NULL;
    hfs_get_catalog_btree(&catalog, hfs);
    hfs_cnid_t cnid = catalog->headerRecord.firstLeafNode;
    
    while (1) {
        BTreeNodePtr node = NULL;
        if ( BTGetNode(&node, catalog, cnid) < 0) {
            perror("get node");
            die(1, "There was an error fetching node %d", cnid);
        }
        
        // Process node
        debug("Processing node %d", cnid); summary.nodeCount++;
        
        FOR_UNTIL(recNum, node->nodeDescriptor->numRecords) {
            
            summary.recordCount++;
            
            BTreeKeyPtr recordKey = NULL;
            Bytes recordValue = NULL;
            btree_get_record(&recordKey, &recordValue, node, recNum);

            HFSPlusCatalogRecord* record = (HFSPlusCatalogRecord*)recordValue;
            switch (record->record_type) {
                case kHFSPlusFileRecord:
                {
                    summary.fileCount++;
                    
                    HFSPlusCatalogFile *file = &record->catalogFile;
                    
                    // hard links
                    if (HFSPlusCatalogFileIsHardLink(record)) { summary.hardLinkFileCount++; continue; }
                    if (HFSPlusCatalogFolderIsHardLink(record)) { summary.hardLinkFolderCount++; continue; }
                    
                    // symlink
                    if (HFSPlusCatalogRecordIsSymLink(record)) { summary.symbolicLinkCount++; continue; }
                    
                    // alias
                    if (HFSPlusCatalogRecordIsAlias(record)) { summary.aliasCount++; continue; }
                    
                    // invisible
                    if (file->userInfo.fdFlags & kIsInvisible) { summary.invisibleFileCount++; continue; }
                    
                    // file sizes
                    if (file->dataFork.logicalSize == 0 && file->resourceFork.logicalSize == 0) { summary.emptyFileCount++; continue; }
                    
                    if (file->dataFork.logicalSize)
                        generateForkSummary(&summary.dataFork, file, &file->dataFork, HFSDataForkType);
                    
                    if (file->resourceFork.logicalSize)
                        generateForkSummary(&summary.resourceFork, file, &file->resourceFork, HFSResourceForkType);
                    
                    size_t fileSize = file->dataFork.logicalSize + file->resourceFork.logicalSize;
                    
                    if (summary.largestFiles[0].measure < fileSize) {
                        summary.largestFiles[0].measure = fileSize;
                        summary.largestFiles[0].cnid = file->fileID;
                        qsort(summary.largestFiles, 10, sizeof(Rank), compare_ranked_files);
                    }
                    
                    break;
                }
                    
                case kHFSPlusFolderRecord:
                {
                    summary.folderCount++;
                    
                    HFSPlusCatalogFolder *folder = &record->catalogFolder;
                    if (folder->valence == 0) summary.emptyDirectoryCount++;
                    
                    break;
                }
                    
                default:
                    break;
            }
            
        }
        
        // Follow link
        cnid = node->nodeDescriptor->fLink;
        if (cnid == 0) { break; } // End Of Line
        
        // Console Status
        static int count = 0; // count of records so far
        static unsigned iter_size = 0; // update the status after this many records
        if (iter_size == 0) {
            iter_size = (catalog->headerRecord.leafRecords/100000);
            if (iter_size == 0) iter_size = 100;
        }

        count += node->nodeDescriptor->numRecords;
//        if (count >= 100000) break;
        
        if ((count % iter_size) == 0) {
            // Update status
            size_t space = summary.dataFork.logicalSpace + summary.resourceFork.logicalSpace;
            char size[128] = {0};
            (void)format_size(size, space, 128);
            
            fprintf(stdout, "\r%0.2f%% (files: %ju; directories: %ju; size: %s)",
                    ((float)count / (float)catalog->headerRecord.leafRecords) * 100.,
                    (intmax_t)summary.fileCount,
                    (intmax_t)summary.folderCount,
                    size
                    );
            fflush(stdout);
        }
        
        btree_free_node(node);
    }
    
    return summary;
}

void generateForkSummary(ForkSummary* forkSummary, const HFSPlusCatalogFile* file, const HFSPlusForkData* fork, hfs_forktype_t type)
{
    forkSummary->count++;
    
    forkSummary->blockCount      += fork->totalBlocks;
    forkSummary->logicalSpace    += fork->logicalSize;
    
    forkSummary->extentRecords++;
    
    if (fork->extents[1].blockCount > 0) forkSummary->fragmentedCount++;
    
    for (int i = 0; i < kHFSPlusExtentDensity; i++) {
        if (fork->extents[i].blockCount > 0) forkSummary->extentDescriptors++; else break;
    }
    
    HFSFork *hfsfork;
    if ( hfsfork_make(&hfsfork, &HIOptions.hfs, *fork, type, file->fileID) < 0 ) {
        die(1, "Could not create fork reference for fileID %u", file->fileID);
    }
        
    ExtentList *extents = hfsfork->extents;
    
    unsigned counter = 1;
    
    Extent *e = NULL;
    TAILQ_FOREACH(e, extents, extents) {
        forkSummary->overflowExtentDescriptors++;
        if (counter && (counter == kHFSPlusExtentDensity) ) {
            forkSummary->overflowExtentRecords++;
            counter = 1;
        } else {
            counter++;
        }
    }
    
    hfsfork_free(hfsfork);
}

ssize_t extractFork(const HFSFork* fork, const String extractPath)
{
    set_hfs_volume(fork->hfs);
    Print("Extracting CNID %u to %s", fork->cnid, extractPath);
    PrintHFSPlusForkData(&fork->forkData, fork->cnid, fork->forkType);
    
    // Open output stream
    FILE* f_out = fopen(extractPath, "w");
    if (f_out == NULL) {
        die(1, "could not open %s", extractPath);
    }
    
    FILE* f_in = fopen_hfsfork((HFSFork*)fork);
    fseeko(f_in, 0, SEEK_SET);
    
    off_t offset        = 0;
    size_t chunkSize    = fork->hfs->block_size*1024; //4-8MB, generally.
    Bytes chunk        = calloc(1, chunkSize);
    ssize_t nbytes      = 0;
    
    size_t totalBytes = 0, bytes = 0;
    char totalStr[100] = {0}, bytesStr[100] = {0};
    totalBytes = fork->logicalSize;
    format_size(totalStr, totalBytes, 100);
    format_size(bytesStr, bytes, 100);
    
    do {
        if ( (nbytes = fread(chunk, 1, chunkSize, f_in)) > 0) {
            bytes += fwrite(chunk, 1, nbytes, f_out);
            format_size(bytesStr, bytes, 100);
            fprintf(stdout, "\rCopying CNID %u to %s: %s of %s copied                ", fork->cnid, extractPath, bytesStr, totalStr);
            fflush(stdout);
        }
    } while (nbytes > 0);
    
    fflush(f_out);
    fclose(f_out);
    fclose(f_in);
    
    FREE(chunk);
    
    Print("\nCopy complete.");
    return offset;
}

void extractHFSPlusCatalogFile(const HFS *hfs, const HFSPlusCatalogFile* file, const String extractPath)
{
    if (file->dataFork.logicalSize > 0) {
        HFSFork *fork = NULL;
        if ( hfsfork_make(&fork, hfs, file->dataFork, HFSDataForkType, file->fileID) < 0 ) {
            die(1, "Could not create fork for fileID %u", file->fileID);
        }
        ssize_t size = extractFork(fork, extractPath);
        if (size < 0) {
            perror("extract");
            die(1, "Extract data fork failed.");
        }
        hfsfork_free(fork);
    }
    if (file->resourceFork.logicalSize > 0) {
        String outputPath;
        ALLOC(outputPath, FILENAME_MAX);
        ssize_t size;
        size = strlcpy(outputPath, extractPath, sizeof(outputPath));
        if (size < 1) die(1, "Could not create destination filename.");
        size = strlcat(outputPath, ".rsrc", sizeof(outputPath));
        if (size < 1) die(1, "Could not create destination filename.");
        
        HFSFork *fork = NULL;
        if ( hfsfork_make(&fork, hfs, file->resourceFork, HFSResourceForkType, file->fileID) < 0 )
            die(1, "Could not create fork for fileID %u", file->fileID);
        
        size = extractFork(fork, extractPath);
        if (size < 0) die(1, "Extract resource fork failed.");
        
        hfsfork_free(fork);
        
        FREE(outputPath);
    }
    // TODO: Merge forks, set attributes ... essentially copy it. Maybe extract in AppleSingle/Double/MacBinary format?
}

String deviceAtPath(String path)
{
#if defined(__APPLE__)
    static struct statfs stats;
    int result = statfs(path, &stats);
    if (result < 0) {
        perror("statfs");
        return NULL;
    }
    
    debug("statfs: from %s to %s", stats.f_mntonname, stats.f_mntfromname);
    
    return stats.f_mntfromname;
#else
    warning("%s: not supported on this platform", __FUNCTION__);
    return NULL;
#endif
}

bool resolveDeviceAndPath(String path_in, String device_out, String path_out)
{
#if defined(__APPLE__)
    String path = realpath(path_in, NULL);
    if (path == NULL) {
        perror("realpath");
        die(1, "Target not found: %s", path_in);
        return false;
    }
    
    struct statfs stats;
    int result = statfs(path_in, &stats);
    if (result < 0) {
        perror("statfs");
        return false;
    }
    
    debug("Device: %s", stats.f_mntfromname);
    debug("Mounted at: %s", stats.f_mntonname);
    debug("Path: %s", path);
    
    String device = stats.f_mntfromname;
    String mountPoint = stats.f_mntonname;
    
    if (strncmp(mountPoint, "/", PATH_MAX) != 0) {
        // Remove mount point from path name (rather blindly, yes)
        size_t len = strlen(mountPoint);
        path = &path[len];
    }
    
    // If we were given a directory that was the root of a mounted volume, set the path to the root of that volume.
    if ( path_in[strlen(path_in) - 1] == '/' && strlen(path) == 0) {
        path[0] = '/';
        path[1] = '\0';
    }
    
    (void)strlcpy(path_out, path, PATH_MAX);
    (void)strlcpy(device_out, device, PATH_MAX);
    
    
    debug("Path in: %s", path_in);
    debug("Device out: %s", device_out);
    debug("Path out: %s", path_out);
    
    return true;
#else
    warning("%s: not supported on this platform", __FUNCTION__);
    return false;
#endif
}

void die(int val, String format, ...)
{
    va_list args;
    va_start(args, format);
    char str[1024];
    vsnprintf(str, 1024, format, args);
    va_end(args);
    
    if (errno > 0) perror(str);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
    
    error(str);
    
#pragma GCC diagnostic pop
    
    exit(val);
}

void loadBTree()
{
    // Load the tree
    if (HIOptions.tree_type == BTreeTypeCatalog) {
        if ( hfs_get_catalog_btree(&HIOptions.tree, &HIOptions.hfs) < 0)
            die(1, "Could not get Catalog B-Tree!");
    
    } else if (HIOptions.tree_type == BTreeTypeExtents) {
        if ( hfs_get_extents_btree(&HIOptions.tree, &HIOptions.hfs) < 0)
            die(1, "Could not get Extents B-Tree!");
    
    } else if (HIOptions.tree_type == BTreeTypeAttributes) {
        if ( hfs_get_attribute_btree(&HIOptions.tree, &HIOptions.hfs) < 0)
            die(1, "Could not get Attribute B-Tree!");
    
    } else if (HIOptions.tree_type == BTreeTypeHotfiles) {
        if ( hfs_get_hotfiles_btree(&HIOptions.tree, &HIOptions.hfs) < 0)
            die(1, "Hotfiles B-Tree not found. Target must be a hard drive with a copy of Mac OS X that has successfully booted to create this file (it will not be present on SSDs).");
    } else {
        die(1, "Unsupported tree: %s", HIOptions.tree_type);
    }
}

int main (int argc, String const *argv)
{
    (void)strlcpy(PROGRAM_NAME, basename(argv[0]), PATH_MAX);
    
    if (argc == 1) usage(1);
    
    if (DEBUG) {
        // Ruin some memory so we crash more predictably.
        Bytes m = malloc(16*1024*1024);
        FREE(m);
    }
    
    setlocale(LC_ALL, "");
    
    DEBUG = getenv("DEBUG");
    
#pragma mark Process Options
    
    /* options descriptor */
    struct option longopts[] = {
        { "help",           no_argument,            NULL,                   'h' },
        { "version",        no_argument,            NULL,                   'v' },
        { "device",         required_argument,      NULL,                   'd' },
        { "yank",           required_argument,      NULL,                   'y' },
        { "volume",         required_argument,      NULL,                   'V' },
        { "volumeheader",   no_argument,            NULL,                   'r' },
        { "node",           required_argument,      NULL,                   'n' },
        { "btree",          required_argument,      NULL,                   'b' },
        { "output",         required_argument,      NULL,                   'o' },
        { "summary",        no_argument,            NULL,                   's' },
        { "path",           required_argument,      NULL,                   'p' },
        { "path_abs",       required_argument,      NULL,                   'P' },
        { "cnid",           required_argument,      NULL,                   'c' },
        { "list",           no_argument,            NULL,                   'l' },
        { "journal",        no_argument,            NULL,                   'j' },
        { "disk-info",      no_argument,            NULL,                   'D' },
        { "freespace",      no_argument,            NULL,                   '0' },
        { "si",             no_argument,            NULL,                   'S' },
        { NULL,             0,                      NULL,                   0   }
    };
    
    /* short options */
    String shortopts = "0ShvjlrsDd:n:b:p:P:F:V:c:o:y:";
    
    char opt;
    while ((opt = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (opt) {
                // Unknown option
            case '?': exit(1);
                
                // Missing argument
            case ':': exit(1);
                
                // Value set or index returned
            case 0: break;
                
            case 'S': output_set_uses_decimal_blocks(true); break;
            
            case 'l': set_mode(HIModeListFolder); break;
                
            case 'v': show_version(); break;       // Exits
                
            case 'h': usage(0); break;                              // Exits
                
            case 'j': set_mode(HIModeShowJournalInfo); break;
                
            case 'D': set_mode(HIModeShowDiskInfo); break;
                
                // Set device with path to file or device
            case 'd': (void)strlcpy(HIOptions.device_path, optarg, PATH_MAX); break;
                
            case 'V':
            {
                String str = deviceAtPath(optarg);
                (void)strlcpy(HIOptions.device_path, str, PATH_MAX);
                if (HIOptions.device_path == NULL) fatal("Unable to determine device. Does the path exist?");
            }
                break;
                
            case 'r': set_mode(HIModeShowVolumeInfo); break;
                
            case 'p':
                set_mode(HIModeShowPathInfo);
                char device[PATH_MAX] = {0}, file[PATH_MAX] = {0};
                
                bool success = resolveDeviceAndPath(optarg, device, file);
                
                if (device != NULL) (void)strlcpy(HIOptions.device_path, device, PATH_MAX);
                if (file != NULL)   (void)strlcpy(HIOptions.file_path, file, PATH_MAX);
                
                if ( !success || !strlen(HIOptions.device_path ) ) fatal("Device could not be determined. Specify manually with -d/--device.");
                if ( !strlen(HIOptions.file_path) ) fatal("Path must be an absolute path from the root of the target filesystem.");
                
                break;
                
            case 'P':
                set_mode(HIModeShowPathInfo);
                (void)strlcpy(HIOptions.file_path, optarg, PATH_MAX);
                if (HIOptions.file_path[0] != '/') fatal("Path given to -P/--path_abs must be an absolute path from the root of the target filesystem.");
                break;
                
            case 'b':
                set_mode(HIModeShowBTreeInfo);
                if (strcmp(optarg, BTreeOptionAttributes) == 0) HIOptions.tree_type = BTreeTypeAttributes;
                else if (strcmp(optarg, BTreeOptionCatalog) == 0) HIOptions.tree_type = BTreeTypeCatalog;
                else if (strcmp(optarg, BTreeOptionExtents) == 0) HIOptions.tree_type = BTreeTypeExtents;
                else if (strcmp(optarg, BTreeOptionHotfiles) == 0) HIOptions.tree_type = BTreeTypeHotfiles;
                else fatal("option -b/--btree must be one of: attributes, catalog, extents, or hotfiles (not %s).", optarg);
                break;
                
            case 'F':
                set_mode(HIModeShowCatalogRecord);
                
                ZERO_STRUCT(HIOptions.record_filename);
                HIOptions.record_parent = 0;
                
                String option    = strdup(optarg);
                String parent    = strsep(&option, ":");
                String filename  = strsep(&option, ":");
                
                if (parent && strlen(parent)) sscanf(parent, "%u", &HIOptions.record_parent);
                if (filename) (void)strlcpy(HIOptions.record_filename, filename, PATH_MAX);
                
                FREE(option);
                
                if (HIOptions.record_filename == NULL || HIOptions.record_parent == 0) fatal("option -F/--fsspec requires a parent ID and file (eg. 2:.journal)");
                
                break;
                
            case 'y':
                set_mode(HIModeYankFS);
                (void)strlcpy(HIOptions.extract_path, optarg, PATH_MAX);
                break;
                
            case 'o':
                set_mode(HIModeExtractFile);
                (void)strlcpy(HIOptions.extract_path, optarg, PATH_MAX);
                break;
                
            case 'n':
                set_mode(HIModeShowBTreeNode);
                
                int count = sscanf(optarg, "%u", &HIOptions.node_id);
                if (count == 0) fatal("option -n/--node requires a numeric argument");
                
                break;
                
            case 'c':
            {
                set_mode(HIModeShowCNID);
                
                int count = sscanf(optarg, "%u", &HIOptions.cnid);
                if (count == 0) fatal("option -c/--cnid requires a numeric argument");
                
                break;
            }
                
            case 's': set_mode(HIModeShowSummary); break;
                
            case '0': set_mode(HIModeFreeSpace); break;
            
            default: debug("Unhandled argument '%c' (default case).", opt); usage(1); break;
        };
    };
    
#pragma mark Prepare Input and Outputs
    
    // If no device path was given, use the volume of the CWD.
    if ( EMPTY_STRING(HIOptions.device_path) ) {
        info("No device specified. Trying to use the root device.");
        
        char device[PATH_MAX];
        (void)strlcpy(device, deviceAtPath("."), PATH_MAX);
        if (device != NULL)
            (void)strlcpy(HIOptions.device_path, device, PATH_MAX);
        
        if ( EMPTY_STRING(HIOptions.device_path) ) {
            error("Device could not be determined. Specify manually with -d/--device.");
            exit(1);
        }
        debug("Found device %s", HIOptions.device_path);
    }
    
    // Load the device
    Volume *vol = NULL;
OPEN:
    vol = vol_qopen(HIOptions.device_path);
    if (vol == NULL) {
        if (errno == EBUSY) {
            // If the device is busy, see if we can use the raw disk instead (and aren't already).
            if (strstr(HIOptions.device_path, "/dev/disk") != NULL) {
                String newDevicePath;
                ALLOC(newDevicePath, PATH_MAX + 1);
                (void)strlcat(newDevicePath, "/dev/rdisk", PATH_MAX);
                (void)strlcat(newDevicePath, &HIOptions.device_path[9], PATH_MAX);
                info("Device %s busy. Trying the raw disk instead: %s", HIOptions.device_path, newDevicePath);
                (void)strlcpy(HIOptions.device_path, newDevicePath, PATH_MAX);
                goto OPEN;
            }
            die(errno, "vol_qopen");
            
        } else {
            error("could not open %s", HIOptions.device_path);
            die(errno, "vol_qopen");
        }
    }

#pragma mark Disk Summary
    
    if (check_mode(HIModeShowDiskInfo)) {
        volumes_dump(vol);
    } else {
        volumes_load(vol);
    }

#pragma mark Find HFS Volume
    
    // Device loaded. Find the first HFS+ filesystem.
    printf("%p\n", vol);
    Volume *tmp = hfs_find(vol);
    if (tmp == NULL) {
        // No HFS Plus volumes found.
        die(1, "No HFS+ filesystems found.");
    } else {
        vol = tmp;
    }
    
    if (hfs_open(&HIOptions.hfs, vol) < 0) {
        die(1, "hfs_attach");
    }
    
    uid_t uid = 99;
    gid_t gid = 99;
    
    // If extracting, determine the UID to become by checking the owner of the output directory (so we can create any requested files later).
    if (check_mode(HIModeExtractFile) || check_mode(HIModeYankFS)) {
        String dir = strdup(dirname(HIOptions.extract_path));
        if ( !strlen(dir) ) {
            die(1, "Output file directory does not exist: %s", dir);
        }
        
        struct stat dirstat = {0};
        int result = stat(dir, &dirstat);
        if (result == -1) {
            die(errno, "stat");
        }
        
        uid = dirstat.st_uid;
        gid = dirstat.st_gid;
        free(dir);
    }

#pragma mark Drop Permissions
    
    // If we're root, drop down.
    if (geteuid() == 0) {
        debug("Dropping privs");
        if (setegid(gid) != 0) die(1, "Failed to drop group privs.");
        if (seteuid(uid) != 0) die(1, "Failed to drop user privs.");
        info("Was running as root.  Now running as %u/%u.", geteuid(), getegid());
    }
    
#pragma mark Yank FS
    // Pull the essential files from the disk for inspection or transport.
	if (check_mode(HIModeYankFS)) {
        if (HIOptions.hfs.vol == NULL) {
            error("No HFS+ volume detected; can't extract FS.");
            goto NOPE;
        }
        
		char path[PATH_MAX] = "";
		strncpy(path, HIOptions.extract_path, PATH_MAX);
        
        int result = mkdir(HIOptions.extract_path, 0777);
        if (result < 0 && errno != EEXIST) {
            die(0, "mkdir");
        }
		
		char headerPath         [PATH_MAX] = "";
		char allocationPath     [PATH_MAX] = "";
		char extentsPath        [PATH_MAX] = "";
		char catalogPath        [PATH_MAX] = "";
		char attributesPath     [PATH_MAX] = "";
		char startupPath        [PATH_MAX] = "";
		char hotfilesPath       [PATH_MAX] = "";
		char journalBlockPath   [PATH_MAX] = "";
        char journalPath        [PATH_MAX] = "";
        
        strncpy(headerPath,         HIOptions.extract_path,         PATH_MAX);
        strncpy(allocationPath,     headerPath,                     PATH_MAX);
        strncpy(extentsPath,        headerPath,                     PATH_MAX);
        strncpy(catalogPath,        headerPath,                     PATH_MAX);
        strncpy(attributesPath,     headerPath,                     PATH_MAX);
        strncpy(startupPath,        headerPath,                     PATH_MAX);
        strncpy(hotfilesPath,       headerPath,                     PATH_MAX);
        strncpy(journalBlockPath,   headerPath,                     PATH_MAX);
        strncpy(journalPath,        headerPath,                     PATH_MAX);
        
        strncat(headerPath,         "/header.block",                PATH_MAX);
        strncat(allocationPath,     "/allocation.bmap",             PATH_MAX);
        strncat(extentsPath,        "/extents.btree",               PATH_MAX);
        strncat(catalogPath,        "/catalog.btree",               PATH_MAX);
        strncat(attributesPath,     "/attributes.btree",            PATH_MAX);
        strncat(startupPath,        "/startupFile",                 PATH_MAX);
        strncat(hotfilesPath,       "/hotfiles.btree",              PATH_MAX);
        strncat(journalBlockPath,   "/journal.block",               PATH_MAX);
        strncat(journalPath,        "/journal.buf",                 PATH_MAX);

        HFSFork *fork = NULL;

        // Volume Header Blocks
        unsigned sectorCount = 16;
        size_t readSize = vol->sector_size*sectorCount;
        
        Bytes buf = calloc(1, readSize);
		int nbytes = 0;
		nbytes = vol_read(vol, (Bytes)buf, readSize, 0);
		if (nbytes < 0) die(0, "reading volume header");
        
        FILE *fp = fopen(headerPath, "w");
        (void)fwrite(buf, 1, readSize, fp);
        fclose(fp);
		
        struct {
            HFSPlusForkData forkData;
            bt_nodeid_t     cnid;
            char            *path;
        } files[5] = {
            // Extents Overflow
            { HIOptions.hfs.vh.extentsFile,     kHFSExtentsFileID,      extentsPath},
            // Catalog
            { HIOptions.hfs.vh.catalogFile,     kHFSCatalogFileID,      catalogPath},
            // Allocation Bitmap (HFS+)
            { HIOptions.hfs.vh.allocationFile,  kHFSAllocationFileID,   allocationPath},
            // Startup (HFS+)
            { HIOptions.hfs.vh.startupFile,     kHFSStartupFileID,      startupPath},
            // Attributes (HFS+)
            { HIOptions.hfs.vh.attributesFile,  kHFSAttributesFileID,   attributesPath},
        };
        
        FOR_UNTIL(i, 5) {
            if (files[i].forkData.logicalSize < 1) continue;
            
            hfsfork_make(&fork, &HIOptions.hfs, files[i].forkData, HFSDataForkType, files[i].cnid);
            (void)extractFork(fork, files[i].path);
            hfsfork_free(fork);
            fork = NULL;
        }
        
        // Hotfiles
        HFSPlusCatalogRecord catalogRecord = {0};
        
        if ( HFSPlusGetCatalogInfoByPath(NULL, &catalogRecord, "/.hotfiles.btree", &HIOptions.hfs) ) {
            hfsfork_make(&fork, &HIOptions.hfs, catalogRecord.catalogFile.dataFork, HFSDataForkType, catalogRecord.catalogFile.fileID);
            (void)extractFork(fork, hotfilesPath);
            hfsfork_free(fork);
            fork = NULL;
        }
        
        // Journal Info Block
        if ( HFSPlusGetCatalogInfoByPath(NULL, &catalogRecord, "/.journal_info_block", &HIOptions.hfs) ) {
            hfsfork_make(&fork, &HIOptions.hfs, catalogRecord.catalogFile.dataFork, HFSDataForkType, catalogRecord.catalogFile.fileID);
            (void)extractFork(fork, journalBlockPath);
            hfsfork_free(fork);
            fork = NULL;
        }
        
        // Journal File
        if ( HFSPlusGetCatalogInfoByPath(NULL, &catalogRecord, "/.journal", &HIOptions.hfs) ) {
            hfsfork_make(&fork, &HIOptions.hfs, catalogRecord.catalogFile.dataFork, HFSDataForkType, catalogRecord.catalogFile.fileID);
            (void)extractFork(fork, journalPath);
            hfsfork_free(fork);
            fork = NULL;
        }
	}
    
NOPE:
    
#pragma mark Load Volume Data
    
    set_hfs_volume(&HIOptions.hfs); // Set the context for certain output_hfs.h calls.
        
#pragma mark Volume Requests
    
    // Always detail what volume we're working on at the very least
    PrintVolumeInfo(&HIOptions.hfs);
    
    // Default to volume info if there are no other specifiers.
    if (HIOptions.mode == 0) set_mode(HIModeShowVolumeInfo);
    
    // Volume summary
    if (check_mode(HIModeShowSummary)) {
        debug("Printing summary.");
        VolumeSummary summary = generateVolumeSummary(&HIOptions.hfs);
        PrintVolumeSummary(&summary);
    }
    
    // Show volume info
    if (check_mode(HIModeShowVolumeInfo)) {
        debug("Printing volume header.");
        PrintVolumeHeader(&HIOptions.hfs.vh);
    }
    
    // Journal info
    if (check_mode(HIModeShowJournalInfo)) {
        if (HIOptions.hfs.vh.attributes & kHFSVolumeJournaledMask) {
            if (HIOptions.hfs.vh.journalInfoBlock != 0) {
                JournalInfoBlock block = {0};
                bool success = hfs_get_JournalInfoBlock(&block, &HIOptions.hfs);
                if (!success) die(1, "Could not get the journal info block!");
                PrintJournalInfoBlock(&block);
                
                journal_header header = {0};
                success = hfs_get_journalheader(&header, block, &HIOptions.hfs);
                if (!success) die(1, "Could not get the journal header!");
                PrintJournalHeader(&header);
                
            } else {
                warning("Consistency error: volume attributes indicate it is journaled but the journal info block is empty!");
            }
        }
    }
    
#pragma mark Allocation Requests
    // Show volume's free space extents
    if (check_mode(HIModeFreeSpace)) {
        debug("Show free space.");
        showFreeSpace();
    }
    
#pragma mark Catalog Requests

    // Show a path's series of records
    if (check_mode(HIModeShowPathInfo)) {
        debug("Show path info.");
        showPathInfo();
    }
    
    // Show a catalog record by FSSpec
    if (check_mode(HIModeShowCatalogRecord)) {
        debug("Finding catalog record for %d:%s", HIOptions.record_parent, HIOptions.record_filename);
        FSSpec spec = {
            .hfs = &HIOptions.hfs,
            .parentID = HIOptions.record_parent,
            .name = strtohfsuc(HIOptions.record_filename)
        };
        showCatalogRecord(spec, false);
    }
    
    // Show a catalog record by its CNID (file/folder ID)
    if (check_mode(HIModeShowCNID)) {
        debug("Showing CNID %d", HIOptions.cnid);
        FSSpec spec = {
            .hfs = &HIOptions.hfs,
            .parentID = HIOptions.cnid,
            .name = {0}
        };
        showCatalogRecord(spec, true);
    }
    
#pragma mark B-Tree Requests

    // Show B-Tree info
    if (check_mode(HIModeShowBTreeInfo)) {
        debug("Printing tree info.");
        
        loadBTree();
        
        if (HIOptions.tree->treeID == kHFSCatalogFileID) {
            BeginSection("Catalog B-Tree Header");
            
        } else if (HIOptions.tree->treeID == kHFSExtentsFileID) {
            BeginSection("Extents B-Tree Header");
            
        } else if (HIOptions.tree->treeID == kHFSAttributesFileID) {
            BeginSection("Attributes B-Tree Header");
            
        } else if (HIOptions.tree_type == BTreeTypeHotfiles) {
            BeginSection("Hotfiles B-Tree Header");
            
        } else {
            die(1, "Unknown tree type: %d", HIOptions.tree_type);
        }
        
        PrintTreeNode(HIOptions.tree, 0);
        EndSection();
    }
    
    // Show a B-Tree node by ID
    if (check_mode(HIModeShowBTreeNode)) {
        debug("Printing tree node.");
        
        loadBTree();
        
        if (HIOptions.tree_type == BTreeTypeCatalog) {
            BeginSection("Catalog B-Tree Node %d", HIOptions.node_id);
            
        } else if (HIOptions.tree_type == BTreeTypeExtents) {
            BeginSection("Extents B-Tree Node %d", HIOptions.node_id);
            
        } else if (HIOptions.tree_type == BTreeTypeAttributes) {
            BeginSection("Attributes B-Tree Node %d", HIOptions.node_id);
            
        } else if (HIOptions.tree_type == BTreeTypeHotfiles) {
            BeginSection("Hotfiles B-Tree Node %d", HIOptions.node_id);
            
        } else {
            die(1, "Unknown tree type: %s", HIOptions.tree_type);
        }
        
        bool showHex = 0;
        
        if (showHex) {
            size_t length = HIOptions.tree->headerRecord.nodeSize;
            off_t offset = length * HIOptions.node_id;
            Bytes buf = calloc(1, length);
            fpread(HIOptions.tree->fp, buf, length, offset);
            VisualizeData(buf, length);
            
        } else {
            PrintTreeNode(HIOptions.tree, HIOptions.node_id);
        }
        
        EndSection();
        
    }
    
#pragma mark Extract File Requests

    // Extract any found files (not complete; FIXME: dropping perms breaks right now)
    if (check_mode(HIModeExtractFile)) {
        if (HIOptions.extract_HFSFork == NULL && HIOptions.tree && HIOptions.tree->treeID != 0) {
            HFSFork *fork;
            hfsfork_get_special(&fork, &HIOptions.hfs, HIOptions.tree->treeID);
            HIOptions.extract_HFSFork = fork;
        }
        
        // Extract any found data, if requested.
        if (HIOptions.extract_path != NULL && strlen(HIOptions.extract_path) != 0) {
            debug("Extracting data.");
            if (HIOptions.extract_HFSPlusCatalogFile.fileID != 0) {
                extractHFSPlusCatalogFile(&HIOptions.hfs, &HIOptions.extract_HFSPlusCatalogFile, HIOptions.extract_path);
            } else if (HIOptions.extract_HFSFork != NULL) {
                extractFork(HIOptions.extract_HFSFork, HIOptions.extract_path);
            }
        }
    }
    
    // Clean up
    hfs_close(&HIOptions.hfs); // also perorms vol_close(vol), though perhaps it shouldn't?
    
    debug("Clean exit.");
    
    return 0;
}

