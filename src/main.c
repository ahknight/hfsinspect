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

void            die                         (int val, char* format, ...)
    __attribute__(( noreturn ));

void            set_mode                    (int mode);
void            clear_mode                  (int mode);
bool            check_mode                  (int mode);

void            usage                       (int status)
    __attribute__(( noreturn ));
void            show_version                ()
    __attribute__(( noreturn ));

void            showPathInfo                (void);
void            showCatalogRecord           (bt_nodeid_t parent, HFSUniStr255 filename, bool follow);

int             compare_ranked_files        (const void * a, const void * b)
    __attribute__(( nonnull ));
VolumeSummary   generateVolumeSummary       (HFS* hfs)
    __attribute__(( nonnull ));
void            generateForkSummary         (ForkSummary* forkSummary, const HFSPlusCatalogFile* file, const HFSPlusForkData* fork, hfs_forktype_t type)
    __attribute__(( nonnull ));

ssize_t         extractFork                 (const HFSFork* fork, const char* extractPath)
    __attribute__(( nonnull ));
void            extractHFSPlusCatalogFile   (const HFS *hfs, const HFSPlusCatalogFile* file, const char* extractPath)
    __attribute__(( nonnull ));

char*           deviceAtPath                (char*)
    __attribute__(( nonnull ));
bool            resolveDeviceAndPath        (char* path_in, char* device_out, char* path_out)
    __attribute__(( nonnull ));


#pragma mark - Variable Definitions

char            PROGRAM_NAME[PATH_MAX];

const char*     BTreeOptionCatalog          = "catalog";
const char*     BTreeOptionExtents          = "extents";
const char*     BTreeOptionAttributes       = "attributes";
const char*     BTreeOptionHotfile          = "hotfile";

typedef enum BTreeTypes {
    BTreeTypeCatalog = 0,
    BTreeTypeExtents,
    BTreeTypeAttributes,
    BTreeTypeHotfile
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
    char* help = "[-hvjs] [-d file] [-V volumepath] [ [-b btree] [-n nodeid] ] [-p path] [-P absolutepath] [-F parent:name] [-o file] [--version] [path]\n"
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
    "    -b NAME,    --btree NAME    Specify which HFS+ B-Tree to work with. Supported options: attributes, catalog, extents, or hotfile. \n"
    "    -n ID,      --node ID       Dump an HFS+ B-Tree node by ID (must specify tree with -b). \n"
    "    -r,         --volumeheader  Dump the volume header. \n"
    "    -j,         --journal       Dump the volume's journal info block structure. \n"
    "    -c CNID,    --cnid CNID     Lookup and display a record by its catalog node ID. \n"
    "    -l,         --list          If the specified FSOB is a folder, list the contents. \n"
    "    -D,         --disk-info     Show any available information about the disk, including partitions and volume headers.\n"
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

void showPathInfo(void)
{
    debug("Finding records for path: %s", HIOptions.file_path);
    
    if (strlen(HIOptions.file_path) == 0) {
        error("Path is required.");
        exit(1);
    }
    
    char* file_path = strdup(HIOptions.file_path);
    char* segment = NULL;
    char last_segment[PATH_MAX] = "";
    
    HFSPlusCatalogFile *fileRecord = NULL;
    uint32_t parentID = kHFSRootFolderID, last_parentID = 0;
    
    while ( (segment = strsep(&file_path, "/")) != NULL ) {
        debug("Segment: %d:%s", parentID, segment);
        (void)strlcpy(last_segment, segment, PATH_MAX);
        last_parentID = parentID;
        
        BTreeNodePtr node;
        BTRecNum recordID = 0;
        
        HFSPlusCatalogKey key;
        HFSPlusCatalogRecord catalogRecord;
        
        HFSUniStr255 search_string = strtohfsuc(segment);
        int8_t result = hfs_catalog_find_record(&node, &recordID, &HIOptions.hfs, parentID, search_string);
        
        if (result) {
            debug("Record found:");
            
            debug("%s: parent: %d (node %d; record %d)", segment, parentID, node->nodeNumber, recordID);
            if (strlen(segment) == 0) continue;
            
            int kind = hfs_get_catalog_leaf_record(&key, &catalogRecord, node, recordID);
            switch (kind) {
                case kHFSPlusFolderRecord:
                {
                    parentID = catalogRecord.catalogFolder.folderID;
                    break;
                }
                    
                case kHFSPlusFileRecord:
                {
                    fileRecord = &catalogRecord.catalogFile;
                    break;
                }
                    
                case kHFSPlusFolderThreadRecord:
                case kHFSPlusFileThreadRecord:
                {
                    parentID = catalogRecord.catalogThread.parentID;
                    break;
                }
                    
                default:
                    error("Didn't change the parent (%d).", kind);
                    break;
            }
        } else {
            die(1, "Path segment not found: %d:%s", parentID, segment);
            
        }
        
    }
    
    if (last_segment != NULL) {
        showCatalogRecord(last_parentID, strtohfsuc(last_segment), false);
    }
    
    if (fileRecord != NULL) {
        HIOptions.extract_HFSPlusCatalogFile = *fileRecord;
    }
    
    FREE_BUFFER(file_path);
}

void showCatalogRecord(bt_nodeid_t parent, HFSUniStr255 filename, bool follow)
{
    hfs_wc_str filename_str;
    hfsuctowcs(filename_str, &filename);
    
    debug("Finding catalog record for %d:%ls", parent, filename_str);
    
    BTreeNodePtr node;
    BTRecNum recordID = 0;
    
    int8_t found = hfs_catalog_find_record(&node, &recordID, &HIOptions.hfs, parent, filename);
    
    if (found) {
        HFSPlusCatalogKey record_key;
        HFSPlusCatalogRecord catalogRecord;
        
        int type = hfs_get_catalog_leaf_record(&record_key, &catalogRecord, node, recordID);
        
        if (type == kHFSPlusFileThreadRecord || type == kHFSPlusFolderThreadRecord) {
            if (follow) {
                debug("Following thread for %d:%ls", parent, filename_str);
                showCatalogRecord(catalogRecord.catalogThread.parentID, catalogRecord.catalogThread.nodeName, follow);
                return;
                
            } else {
                if (type == kHFSPlusFolderThreadRecord && check_mode(HIModeListFolder)) {
                    PrintFolderListing(record_key.parentID);
                } else {
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
            PrintNodeRecord(node, recordID);
            
            // Set extract file
            HIOptions.extract_HFSPlusCatalogFile = catalogRecord.catalogFile;
            
        } else if (type == kHFSPlusFolderRecord) {
            if (check_mode(HIModeListFolder)) {
                PrintFolderListing(catalogRecord.catalogFolder.folderID);
            } else {
                PrintNodeRecord(node, recordID);
            }
            
        } else {
            error("Unknown record type: %d", type);
        }
        
    } else {
        error("Record not found: %d:%ls", parent, filename_str);
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
                    if (hfs_catalog_record_is_file_hard_link(record)) { summary.hardLinkFileCount++; continue; }
                    if (hfs_catalog_record_is_directory_hard_link(record)) { summary.hardLinkFolderCount++; continue; }
                    
                    // symlink
                    if (hfs_catalog_record_is_symbolic_link(record)) { summary.symbolicLinkCount++; continue; }
                    
                    // alias
                    if (hfs_catalog_record_is_alias(record)) { summary.aliasCount++; continue; }
                    
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
            (void)format_size(size, space, false, 128);
            
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

ssize_t extractFork(const HFSFork* fork, const char* extractPath)
{
    print("Extracting fork to %s", extractPath);
//    PrintHFSPlusForkData(&fork->forkData, fork->cnid, fork->forkType);
    
    // Open output stream
    FILE* f_out = fopen(extractPath, "w");
    if (f_out == NULL) {
        die(1, "could not open %s", extractPath);
    }
    
    FILE* f_in = fopen_hfsfork((HFSFork*)fork);
    fseeko(f_in, 0, SEEK_SET);
    
    off_t offset        = 0;
    size_t chunkSize    = fork->hfs->block_size*1024;
    char* chunk         = calloc(1, chunkSize);
    ssize_t nbytes      = 0;
    
    size_t totalBytes, bytes;
    char totalStr[100] = {0}, bytesStr[100] = {0};
    totalBytes = fork->logicalSize;
    bytes = 0;
    format_size(totalStr, totalBytes, 0, 100);
    format_size(bytesStr, bytes, 0, 100);
    
    do {
        nbytes = fread(chunk, sizeof(char), chunkSize, f_in);
        if (nbytes) {
            nbytes = fwrite(chunk, sizeof(char), nbytes, f_out);
        }
        bytes += nbytes;
        format_size(bytesStr, bytes, 0, 100);
        fprintf(stdout, "\r%s of %s copied", bytesStr, totalStr);
        fflush(stdout);
        
    } while (nbytes > 0);
    
    fflush(f_out);
    fclose(f_out);
    fclose(f_in);
    
    FREE_BUFFER(chunk);
    
    print("Done.");
    return offset;
}

void extractHFSPlusCatalogFile(const HFS *hfs, const HFSPlusCatalogFile* file, const char* extractPath)
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
        char* outputPath;
        INIT_BUFFER(outputPath, FILENAME_MAX);
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
        
        FREE_BUFFER(outputPath);
    }
    // TODO: Merge forks, set attributes ... essentially copy it. Maybe extract in AppleSingle/Double/MacBinary format?
}

char* deviceAtPath(char* path)
{
#if defined(__APPLE__)
    static struct statfs stats;
    int result = statfs(path, &stats);
    if (result < 0) {
        perror("statfs");
        return NULL;
    }
    
    debug("statfs: from %s to %s", stats.f_mntfromname, stats.f_mntonname);
    
    return stats.f_mntfromname;
#else
    warning("%s: not supported on this platform", __FUNCTION__);
    return NULL;
#endif
}

bool resolveDeviceAndPath(char* path_in, char* device_out, char* path_out)
{
#if defined(__APPLE__)
    char* path = realpath(path_in, NULL);
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
    
    char* device = stats.f_mntfromname;
    char* mountPoint = stats.f_mntonname;
    
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

void die(int val, char* format, ...)
{
    va_list args;
    va_start(args, format);
    char str[1024];
    vsnprintf(str, 1024, format, args);
    va_end(args);
    
    if (errno > 0) perror(str);
    
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
    error(str);
#pragma clang diagnostic pop
    
    exit(val);
}

void loadBTree()
{
    // Load the tree
    if (HIOptions.tree_type == BTreeTypeCatalog) {
        if ( hfs_get_catalog_btree(&HIOptions.tree, &HIOptions.hfs) < 0)
            die(1, "Could not get Catalog BTree!");
    
    } else if (HIOptions.tree_type == BTreeTypeExtents) {
        if ( hfs_get_extents_btree(&HIOptions.tree, &HIOptions.hfs) < 0)
            die(1, "Could not get Extents BTree!");
    
    } else if (HIOptions.tree_type == BTreeTypeAttributes) {
        if ( hfs_get_attribute_btree(&HIOptions.tree, &HIOptions.hfs) < 0)
            die(1, "Could not get Attribute BTree!");
    
    } else if (HIOptions.tree_type == BTreeTypeHotfile) {
        BTreeNodePtr node = NULL;
        BTRecNum recordID = 0;
        bt_nodeid_t parentfolder = kHFSRootFolderID;
        HFSUniStr255 name = wcstohfsuc(L".hotfiles.btree");
        int found = hfs_catalog_find_record(&node, &recordID, &HIOptions.hfs, parentfolder, name);
        if (found != 1) {
            error("Hotfiles btree not found. Target must be a hard drive with a copy of Mac OS X that has successfully booted to create this file (it will not be present on SSDs).");
            exit(1);
        }
        BTreeKeyPtr recordKey = NULL;
        Bytes recordValue = NULL;
        btree_get_record(&recordKey, &recordValue, node, recordID);

        HFSPlusCatalogRecord* record = (HFSPlusCatalogRecord*)recordValue;
        HFSFork *fork = NULL;
        if ( hfsfork_make(&fork, &HIOptions.hfs, record->catalogFile.dataFork, 0x00, record->catalogFile.fileID) < 0 ) {
            die(1, "Could not create fork for fileID %u", record->catalogFile.fileID);
        }
        
        FILE* fp = fopen_hfsfork(fork);
        int result = btree_init(HIOptions.tree, fp);
        if (result < 1) {
            error("Error initializing hotfiles btree.");
            exit(1);
        }
        
    } else {
        die(1, "Unsupported tree: %s", HIOptions.tree_type);
    }
}

int main (int argc, char* const *argv)
{
    (void)strlcpy(PROGRAM_NAME, basename(argv[0]), PATH_MAX);
    
    if (argc == 1) usage(1);
    
    if (DEBUG) {
        // Ruin some memory so we crash more predictably.
        void* m = malloc(16*1024*1024);
        FREE_BUFFER(m);
    }
    
    setlocale(LC_ALL, "");
    
    DEBUG = getenv("DEBUG");
    
#pragma mark Process Options
    
    /* options descriptor */
    struct option longopts[] = {
        { "help",           no_argument,            NULL,                   'h' },
        { "version",        no_argument,            NULL,                   'v' },
        { "device",         required_argument,      NULL,                   'd' },
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
        { NULL,             0,                      NULL,                   0   }
    };
    
    /* short options */
    char* shortopts = "hvjlsDd:n:b:p:P:F:V:c:o:";
    
    char opt;
    while ((opt = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (opt) {
                // Unknown option
            case '?': exit(1);
                
                // Missing argument
            case ':': exit(1);
                
                // Value set or index returned
            case 0: break;
                
            case 'l': set_mode(HIModeListFolder); break;
                
            case 'v': show_version(); break;       // Exits
                
            case 'h': usage(0); break;                              // Exits
                
            case 'j': set_mode(HIModeShowJournalInfo); break;
                
            case 'D': set_mode(HIModeShowDiskInfo); break;
                
                // Set device with path to file or device
            case 'd': (void)strlcpy(HIOptions.device_path, optarg, PATH_MAX); break;
                
            case 'V':
            {
                char* str = deviceAtPath(optarg);
                (void)strlcpy(HIOptions.device_path, str, PATH_MAX);
                if (HIOptions.device_path == NULL) fatal("Unable to determine device. Does the path exist?");
            }
                break;
                
            case 'r': set_mode(HIModeShowVolumeInfo); break;
                
            case 'p':
                set_mode(HIModeShowPathInfo);
                char *device = NULL, *file = NULL;
                
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
                else if (strcmp(optarg, BTreeOptionHotfile) == 0) HIOptions.tree_type = BTreeTypeHotfile;
                else fatal("option -b/--btree must be one of: attributes, catalog, extents, or hotfile (not %s).", optarg);
                break;
                
            case 'F':
                set_mode(HIModeShowCatalogRecord);
                
                ZERO_STRUCT(HIOptions.record_filename);
                HIOptions.record_parent = 0;
                
                char* option    = strdup(optarg);
                char* parent    = strsep(&option, ":");
                char* filename  = strsep(&option, ":");
                
                if (parent && strlen(parent)) sscanf(parent, "%u", &HIOptions.record_parent);
                if (filename) (void)strlcpy(HIOptions.record_filename, filename, PATH_MAX);
                
                FREE_BUFFER(option);
                
                if (HIOptions.record_filename == NULL || HIOptions.record_parent == 0) fatal("option -F/--fsspec requires a parent ID and file (eg. 2:.journal)");
                
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
                char* newDevicePath;
                INIT_BUFFER(newDevicePath, PATH_MAX + 1);
                (void)strlcat(newDevicePath, "/dev/rdisk", PATH_MAX);
                (void)strlcat(newDevicePath, &HIOptions.device_path[9], PATH_MAX);
                info("Device %s busy. Trying the raw disk instead: %s", HIOptions.device_path, newDevicePath);
                (void)strlcpy(HIOptions.device_path, newDevicePath, PATH_MAX);
                goto OPEN;
            }
            perror("vol_qopen");
            exit(errno);
            
        } else {
            error("could not open %s", HIOptions.device_path);
            perror("vol_qopen");
            exit(errno);
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
    
    vol = hfs_find(vol);
    if (vol == NULL) {
        // No HFS Plus volumes found.
        die(1, "No HFS+ filesystems found.");
    }
    
    if (hfs_attach(&HIOptions.hfs, vol) < 0) {
        perror("hfs_attach");
        exit(1);
    }
    
    uid_t uid = 99;
    gid_t gid = 99;
    
    // If extracting, determine the UID to become by checking the owner of the output directory (so we can create any requested files later).
    if (check_mode(HIModeExtractFile)) {
        char* dir = strdup(dirname(HIOptions.extract_path));
        if ( !strlen(dir) ) {
            die(1, "Output file directory does not exist: %s", dir);
        }
        
        struct stat dirstat = {0};
        int result = stat(dir, &dirstat);
        if (result == -1) {
            perror("stat");
            exit(1);
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
    
#pragma mark Catalog Requests

    // Show a path's series of records
    if (check_mode(HIModeShowPathInfo)) {
        debug("Show path info.");
        showPathInfo();
    }
    
    // Show a catalog record by FSSpec
    if (check_mode(HIModeShowCatalogRecord)) {
        debug("Finding catalog record for %d:%s", HIOptions.record_parent, HIOptions.record_filename);
        HFSUniStr255 search_string = strtohfsuc(HIOptions.record_filename);
        showCatalogRecord(HIOptions.record_parent, search_string, false);
    }
    
    // Show a catalog record by its CNID (file/folder ID)
    if (check_mode(HIModeShowCNID)) {
        debug("Showing CNID %d", HIOptions.cnid);
        HFSUniStr255 emptyKey = { 0, {'\0'} };
        showCatalogRecord(HIOptions.cnid, emptyKey, true);
    }
    
#pragma mark BTree Requests

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
            
        } else if (HIOptions.tree_type == BTreeTypeHotfile) {
            BeginSection("Hotfile B-Tree Header");
            
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
            
        } else if (HIOptions.tree_type == BTreeTypeHotfile) {
            BeginSection("Hotfile B-Tree Node %d", HIOptions.node_id);
            
        } else {
            die(1, "Unknown tree type: %s", HIOptions.tree_type);
        }
        
        bool showHex = 0;
        
        if (showHex) {
            size_t length = HIOptions.tree->headerRecord.nodeSize;
            off_t offset = length * HIOptions.node_id;
            char* buf = calloc(1, length);
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
        if (HIOptions.extract_HFSFork == NULL && HIOptions.tree->treeID != 0) {
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
    hfs_close(&HIOptions.hfs);
    
    debug("Clean exit.");
    
    return 0;
}

