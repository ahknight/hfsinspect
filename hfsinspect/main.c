//
//  main.c
//  hfsinspect
//
//  Created by Adam Knight on 5/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <getopt.h>
#include <locale.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <locale.h>
#include <sys/stat.h>

#include "hfs.h"
#include "stringtools.h"

#pragma mark - Function Declarations

void            set_mode                    (int mode);
void            clear_mode                  (int mode);
bool            check_mode                  (int mode);

void            usage                       (int status);
void            show_version                (char* name);

void            showPathInfo                ();
void            showCatalogRecord           (hfs_node_id parent, HFSUniStr255 filename, bool follow);

int             compare_ranked_files        (const Rank * a, const Rank * b);
VolumeSummary   generateVolumeSummary       (HFSVolume* hfs);
void            generateForkSummary         (ForkSummary* forkSummary, const HFSPlusCatalogFile* file, const HFSPlusForkData* fork);

ssize_t         extractFork                 (const HFSFork* fork, const char* extractPath);
void            extractHFSPlusCatalogFile   (const HFSVolume *hfs, const HFSPlusCatalogFile* file, const char* extractPath);

char*           deviceAtPath                (char*);
bool            resolveDeviceAndPath        (char* path_in, char* device_out, char* path_out);


#pragma mark - Variable Definitions

const char*     BTreeOptionCatalog          = "catalog";
const char*     BTreeOptionExtents          = "extents";
const char*     BTreeOptionAttributes       = "attributes";
const char*     BTreeOptionHotfile          = "hotfile";

enum HIModes {
    HIModeShowHelp = 0,
    HIModeShowVersion,
    HIModeShowVolumeInfo,
    HIModeShowJournalInfo,
    HIModeShowSummary,
    HIModeShowBTreeInfo,
    HIModeShowBTreeNode,
    HIModeShowCatalogRecord,
    HIModeShowCNID,
    HIModeShowPathInfo,
    HIModeExtractFile,
    HIModeListFolder,
};

// Configuration context
static struct HIOptions {
    u_int32_t   mode;
    
    char*       device_path;
    char*       file_path;
    
    hfs_node_id record_parent;
    char*       record_filename;
    
    hfs_node_id cnid;
    
    char*               extract_path;
    FILE*               extract_fp;
    HFSPlusCatalogFile  extract_HFSPlusCatalogFile;
    HFSFork             extract_HFSFork;
    
    HFSVolume   hfs;
    const char* tree_type;
    HFSBTree    tree;
    hfs_node_id node_id;
    
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
     -J,        --journalbuffer display detailed information about the journal
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
    char* help = "usage: hfsinspect [-hvjs] [-d file] [-V volumepath] [ [-b btree] [-n nodeid] ] [-p path] [-P absolutepath] [-F parent:name] [-o file] [--version] [path] \n\
\n\
SOURCES: \n\
    hfsinspect will use the root filesystem by default, or the filesystem containing a target file in some cases. If you wish to\n\
    specify a specific device or volume, you can with the following options:\n\
\n\
    -d DEV,     --device DEV    Path to device or file containing a bare HFS+ filesystem (no partition map or HFS wrapper) \n\
    -V VOLUME   --volume VOLUME Use the path to a mounted disk or any file on the disk to use a mounted volume. \n\
\n\
INFO: \n\
    By default, hfsinspect will just show you the volume header and quit.  Use the following options to get more specific data.\n\
\n\
    -h,         --help          Show help and quit. \n\
                --version       Show version information and quit. \n\
    -b NAME,    --btree NAME    Specify which HFS+ B-Tree to work with. Supported options: attributes, catalog, extents, or hotfile. \n\
    -n,         --node ID       Dump an HFS+ B-Tree node by ID (must specify tree with -b). \n\
    -v,         --volumeheader  Dump the volume header. \n\
    -j,         --journalinfo   Dump the volume's journal info block structure. \n\
    -c CNID,    --cnid CNID     Lookup and display a record by its catalog node ID. \n\
    -l,         --list          If the specified FSOB is a folder, list the contents. \n\
\n\
OUTPUT: \n\
    You can optionally have hfsinspect dump any fork it finds as the result of an operation. This includes B-Trees or file forks.\n\
    Use a command like \"-b catalog -o catalog.dump\" to extract the catalog file from the boot drive, for instance.\n\
\n\
    -o PATH,    --output PATH   Use with -b or -p to dump a raw data fork (when used with -b, will dump the HFS+ tree file). \n\
\n\
ENVIRONMENT: \n\
    Set NOCOLOR to hide ANSI colors (TODO: check terminal for support, etc.). \n\
    Set DEBUG to be overwhelmed with useless data. \n\
\n\
";
    fprintf(stderr, "%s", help);
    exit(status);
}

void show_version(char* name)
{
    debug("Version requested.");
    fprintf(stdout, "%s %s\n", name, "1.0");
    exit(0);
}

void showPathInfo()
{
    debug("Finding records for path: %s", HIOptions.file_path);
    
    if (strlen(HIOptions.file_path) == 0) {
        error("Path is required.");
        exit(1);
    }
    
    char* file_path = strdup(HIOptions.file_path);
    char* segment = NULL;
    char* last_segment = NULL;
    
    HFSPlusCatalogFile *fileRecord = NULL;
    u_int32_t parentID = kHFSRootFolderID, last_parentID = 0;
    HFSUniStr255 hfs_str;
    
    while ( (segment = strsep(&file_path, "/")) != NULL ) {
        debug("Segment: %d:%s", parentID, segment);
        if (last_segment != NULL) FREE_BUFFER(last_segment);
        last_segment = strdup(segment);
        last_parentID = parentID;
        
        HFSBTreeNode node;
        u_int32_t recordID = 0;
        
        HFSPlusCatalogKey key;
        HFSPlusCatalogRecord catalogRecord;
        
        HFSUniStr255 search_string = strtohfsuc(segment);
        int8_t result = hfs_catalog_find_record(&node, &recordID, &HIOptions.hfs, parentID, search_string);
        
        if (result) {
            debug("Record found:");
            hfs_str = strtohfsuc(segment);
            
            debug("%s: parent: %d (node %d; record %d)", segment, parentID, node.nodeNumber, recordID);
            if (strlen(segment) == 0) continue;
            
            int kind = hfs_get_catalog_leaf_record(&key, &catalogRecord, &node, recordID);
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
            critical("Path segment not found: %d:%s", parentID, segment);
            
        }
        
    }
    
    if (last_segment != NULL) {
        showCatalogRecord(last_parentID, strtohfsuc(last_segment), false);
    }
    
    if (fileRecord != NULL) {
        HIOptions.extract_HFSPlusCatalogFile = *fileRecord;
    }
}

void showCatalogRecord(hfs_node_id parent, HFSUniStr255 filename, bool follow)
{
    wchar_t* filename_str = hfsuctowcs(&filename);
    
    debug("Finding catalog record for %d:%ls", parent, filename_str);
    
    HFSBTreeNode node;
    u_int32_t recordID = 0;
    
    int8_t found = hfs_catalog_find_record(&node, &recordID, &HIOptions.hfs, parent, filename);
    
    if (found) {
        HFSPlusCatalogKey record_key;
        HFSPlusCatalogRecord catalogRecord;
        
        int type = hfs_get_catalog_leaf_record(&record_key, &catalogRecord, &node, recordID);
        
        if (type == kHFSPlusFileThreadRecord || type == kHFSPlusFolderThreadRecord) {
            if (follow) {
                debug("Following thread for %d:%ls", parent, filename_str);
                FREE_BUFFER(filename_str);
                showCatalogRecord(catalogRecord.catalogThread.parentID, catalogRecord.catalogThread.nodeName, follow);
                return;
                
            } else {
                if (type == kHFSPlusFolderThreadRecord && check_mode(HIModeListFolder)) {
                    PrintFolderListing(record_key.parentID);
                } else {
                    PrintNodeRecord(&node, recordID);
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
            PrintNodeRecord(&node, recordID);
            
            // Set extract file
            HIOptions.extract_HFSPlusCatalogFile = *(HFSPlusCatalogFile*)node.records[recordID].value;
            
        } else if (type == kHFSPlusFolderRecord) {
            if (check_mode(HIModeListFolder)) {
                PrintFolderListing(catalogRecord.catalogFolder.folderID);
            } else {
                PrintNodeRecord(&node, recordID);
            }
            
        } else {
            error("Unknown record type: %d", type);
        }
        
    } else {
        error("Record not found: %d:%ls", parent, filename_str);
    }
    
    FREE_BUFFER(filename_str);
}

int compare_ranked_files(const Rank * a, const Rank * b)
{
    int result = -1;
    
    if (a->measure == b->measure) {
        result = 0;
    } else if (a->measure > b->measure) result = 1;
    
    return result;
}

VolumeSummary generateVolumeSummary(HFSVolume* hfs)
{
    /*
     Walk the leaf catalog nodes and gather various stats about the volume as a whole.
     */
    
    VolumeSummary summary;
    memset(&summary, 0, sizeof(summary));
    
    HFSBTree catalog = hfs_get_catalog_btree(hfs);
    u_int32_t cnid = catalog.headerRecord.firstLeafNode;
    
    while (1) {
        HFSBTreeNode node;
        int8_t result = hfs_btree_get_node(&node, &catalog, cnid);
        if (result < 0) {
            perror("get node");
            critical("There was an error fetching node %d", cnid);
        }
        
        // Process node
        debug("Processing node %d", cnid); summary.nodeCount++;
        for (unsigned recNum = 0; recNum < node.nodeDescriptor.numRecords; recNum++) {
            const HFSBTreeNodeRecord *record = &node.records[recNum]; summary.recordCount++;
            
            // fileCount, folderCount
            u_int8_t type = hfs_get_catalog_record_type(record);
            switch (type) {
                case kHFSPlusFileRecord:
                {
                    summary.fileCount++;
                    
                    HFSPlusCatalogFile *file = ((HFSPlusCatalogFile*)record->value);
                    
                    // hard link
                    if (file->userInfo.fdType == kHardLinkFileType && file->userInfo.fdCreator == kHFSPlusCreator) { summary.hardLinkFileCount++; }
                    
                    // symlink
                    if (file->userInfo.fdType == kSymLinkFileType && file->userInfo.fdCreator == kSymLinkCreator) { summary.symbolicLinkCount++; }
                    
                    {
                        // file alias
                        if (file->userInfo.fdType == 'alis' && file->userInfo.fdCreator == 'MACS') { summary.aliasCount++; }
                        
                        // folder alias
                        else if (file->userInfo.fdType == 'fdrp' && file->userInfo.fdCreator == 'MACS') { summary.aliasCount++; }
                        
                        // catch-all alias
                        else if (file->userInfo.fdFlags & kIsAlias) { summary.aliasCount++; }
                    }
                    
                    // invisible
                    if (file->userInfo.fdFlags & kIsInvisible) { summary.invisibleFileCount++; }
                    
                    if (file->dataFork.logicalSize == 0 && file->resourceFork.logicalSize == 0) {
                        summary.emptyFileCount++;
                        
                    } else {
                        if (file->dataFork.logicalSize) {
                            generateForkSummary(&summary.dataFork, file, &file->dataFork);
                        }
                        
                        if (file->resourceFork.logicalSize) {
                            generateForkSummary(&summary.resourceFork, file, &file->resourceFork);
                        }
                        
                        size_t fileSize = file->dataFork.logicalSize + file->resourceFork.logicalSize;
                        
                        if (summary.largestFiles[0].measure < fileSize) {
                            summary.largestFiles[0].measure = fileSize;
                            summary.largestFiles[0].cnid = file->fileID;
                            qsort(summary.largestFiles, 10, sizeof(Rank), (int(*)(const void*, const void*))compare_ranked_files);
                        }
                        
                        break;
                    }
                    
                case kHFSPlusFolderRecord:
                    {
                        summary.folderCount++;
                        
                        HFSPlusCatalogFolder *folder = ((HFSPlusCatalogFolder*)record->value);
                        
                        // hard link
                        if (folder->flags & kHFSHasLinkChainMask) summary.hardLinkFolderCount++;
                        
                        break;
                    }
                    
                default:
                    break;
                }
                    
            }
            
        }
        // Follow link
        
        cnid = node.nodeDescriptor.fLink;
        if (cnid == 0) { break; } // End Of Line
        
        static int count = 0;
        count += node.nodeDescriptor.numRecords;
//        if (count >= 10000) break;
        
        if ((count % (catalog.headerRecord.leafRecords/10000)) == 0) {
            // Update
            size_t space = summary.dataFork.logicalSpace + summary.resourceFork.logicalSpace;
            char* size = sizeString(space);
            
            fprintf(stdout, "\x1b[G%0.2f%% (files: %llu; directories: %llu; size: %s)",
                    (float)count / (float)catalog.headerRecord.leafRecords * 100.,
                    summary.fileCount,
                    summary.folderCount,
                    size
                    );
            fflush(stdout);
            FREE_BUFFER(size);
        }
    
    }
    
    return summary;
}

void generateForkSummary(ForkSummary* forkSummary, const HFSPlusCatalogFile* file, const HFSPlusForkData* fork)
{
    forkSummary->count++;
    
    forkSummary->blockCount      += fork->totalBlocks;
    forkSummary->logicalSpace    += fork->logicalSize;
    
    forkSummary->extentRecords++;
    
    if (fork->extents[1].blockCount > 0) forkSummary->fragmentedCount++;
    
    for (int i = 0; i < kHFSPlusExtentDensity; i++) {
        if (fork->extents[i].blockCount > 0) forkSummary->extentDescriptors++; else break;
    }
}

ssize_t extractFork(const HFSFork* fork, const char* extractPath)
{
    print("Extracting fork to %s", extractPath);
    PrintHFSPlusForkData(&fork->forkData, fork->cnid, fork->forkType);
    
    FILE* f = fopen(extractPath, "w");
    if (f == NULL) {
        perror("open");
        return -1;
    }
    
    off_t offset        = 0;
    size_t chunkSize    = fork->hfs.vh.blockSize*1024;
    Buffer chunk        = buffer_alloc(chunkSize);
    
    while ( offset < fork->forkData.logicalSize ) {
        info("Remaining: %zu bytes", fork->forkData.logicalSize - offset);
        
        buffer_reset(&chunk);
        
        ssize_t size = hfs_read_fork_range(&chunk, fork, chunkSize, offset);
        if (size < 1) {
            perror("extractFork");
            critical("Read error while extracting range (%llu, %zu)", offset, chunkSize);
            return -1;
        } else {
            fwrite(chunk.data, sizeof(char), size, f);
            offset += size;
        }
    }
    fflush(f);
    fclose(f);
    buffer_free(&chunk);
    
    return offset;
}

void extractHFSPlusCatalogFile(const HFSVolume *hfs, const HFSPlusCatalogFile* file, const char* extractPath)
{
    if (file->dataFork.logicalSize > 0) {
        HFSFork fork = hfsfork_make(hfs, file->dataFork, HFSDataForkType, file->fileID);
        ssize_t size = extractFork(&fork, extractPath);
        if (size < 0) {
            perror("extract");
            critical("Extract data fork failed.");
        }
        hfsfork_free(&fork);
    }
    if (file->resourceFork.logicalSize > 0) {
        char* outputPath;
        INIT_STRING(outputPath, FILENAME_MAX);
        ssize_t size;
        size = strlcpy(outputPath, extractPath, sizeof(outputPath));
        if (size < 1) critical("Could not create destination filename.");
        size = strlcat(outputPath, ".rsrc", sizeof(outputPath));
        if (size < 1) critical("Could not create destination filename.");
        
        HFSFork fork = hfsfork_make(hfs, file->resourceFork, HFSResourceForkType, file->fileID);
        size = extractFork(&fork, extractPath);
        if (size < 0) {
            perror("extract");
            critical("Extract resource fork failed.");
        }
        hfsfork_free(&fork);
        FREE_BUFFER(outputPath);
    }
    // TODO: Merge forks, set attributes ... essentially copy it. Maybe extract in AppleSingle/Double/MacBinary format?
}

char* deviceAtPath(char* path)
{
    struct statfs stats;
    int result = statfs(path, &stats);
    if (result < 0) {
        perror("statfs");
        return NULL;
    }
    
    debug("From: %s", stats.f_mntfromname);
    debug("To: %s", stats.f_mntonname);
    
    char* devicePath = strdup(stats.f_mntfromname);
    return devicePath;
}

bool resolveDeviceAndPath(char* path_in, char* device_out, char* path_out)
{
    char* path = realpath(path_in, NULL);
    if (path == NULL) {
        perror("realpath");
        critical("Target not found: %s", path_in);
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
    
    strlcpy(path_out, path, PATH_MAX);
    strlcpy(device_out, device, PATH_MAX);
    
    
    debug("Path in: %s", path_in);
    debug("Device out: %s", device_out);
    debug("Path out: %s", path_out);
    
    return true;
}

int main (int argc, char* const *argv)
{
    
    if (argc == 1) usage(1);
    
    setlocale(LC_ALL, "");
    
#pragma mark Process Options
    
    /* options descriptor */
    struct option longopts[] = {
        { "help",           no_argument,            NULL,                   'h' },
        { "version",        no_argument,            NULL,                   'Z' },
        { "device",         required_argument,      NULL,                   'd' },
        { "volume",         required_argument,      NULL,                   'V' },
        { "volumeheader",   no_argument,            NULL,                   'v' },
        { "node",           required_argument,      NULL,                   'n' },
        { "btree",          required_argument,      NULL,                   'b' },
        { "output",         required_argument,      NULL,                   'o' },
        { "summary",        no_argument,            NULL,                   's' },
        { "path",           required_argument,      NULL,                   'p' },
        { "path_abs",       required_argument,      NULL,                   'P' },
        { "cnid",           required_argument,      NULL,                   'c' },
        { "list",           no_argument,            NULL,                   'l' },
        { "journalinfo",    no_argument,            NULL,                   'j' },
        { NULL,             0,                      NULL,                   0   }
    };
    
    /* short options */
    char* shortopts = "hd:vjn:b:o:sp:P:F:V:c:l";
    
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
                
            case 'Z': set_mode(HIModeShowVersion); break;
                
            case 'h': set_mode(HIModeShowHelp); break;
                
            case 'j': set_mode(HIModeShowJournalInfo); break;
                
                // Set device with path to file or device
            case 'd': HIOptions.device_path = strdup(optarg); break;
                
            case 'V':
                HIOptions.device_path = deviceAtPath(optarg);
                if (HIOptions.device_path == NULL) fatal("Unable to determine device. Does the path exist?");
                break;
                
            case 'v': set_mode(HIModeShowVolumeInfo); break;
                
            case 'p':
                set_mode(HIModeShowPathInfo);
                
                INIT_STRING(HIOptions.device_path, PATH_MAX);
                INIT_STRING(HIOptions.file_path,   PATH_MAX);
                
                bool success = resolveDeviceAndPath(optarg, HIOptions.device_path, HIOptions.file_path);
                if ( !success || !strlen(HIOptions.device_path ) ) fatal("Device could not be determined. Specify manually with -d/--device.");
                if ( !strlen(HIOptions.file_path) ) fatal("Path must be an absolute path from the root of the target filesystem.");
                
                break;
                
            case 'P':
                set_mode(HIModeShowPathInfo);
                HIOptions.file_path = strdup(optarg);
                if (HIOptions.file_path[0] != '/') fatal("Path given to -P/--path_abs must be an absolute path from the root of the target filesystem.");
                break;
                
            case 'b':
                set_mode(HIModeShowBTreeInfo);
                if (strcmp(optarg, BTreeOptionAttributes) == 0) HIOptions.tree_type = BTreeOptionAttributes;
                else if (strcmp(optarg, BTreeOptionCatalog) == 0) HIOptions.tree_type = BTreeOptionCatalog;
                else if (strcmp(optarg, BTreeOptionExtents) == 0) HIOptions.tree_type = BTreeOptionExtents;
                else if (strcmp(optarg, BTreeOptionHotfile) == 0) HIOptions.tree_type = BTreeOptionHotfile;
                else fatal("option -b/--btree must be one of: attributes, catalog, extents, or hotfile (not %s).", optarg);
                break;
                
            case 'F':
                set_mode(HIModeShowCatalogRecord);
                
                HIOptions.record_filename = NULL;
                HIOptions.record_parent = 0;
                
                char* option    = strdup(optarg);
                char* parent    = strsep(&option, ":");
                char* filename  = strsep(&option, ":");
                
                if (parent && strlen(parent)) sscanf(parent, "%u", &HIOptions.record_parent);
                if (filename) HIOptions.record_filename = strdup(filename);
                
                FREE_BUFFER(option);
                
                if (HIOptions.record_filename == NULL || HIOptions.record_parent == 0) fatal("option -F/--fsspec requires a parent ID and file (eg. 2:.journal)");
                
                break;
                
            case 'o':
                set_mode(HIModeExtractFile);
                HIOptions.extract_path = strdup(optarg);
                break;
                
            case 'n':
                set_mode(HIModeShowBTreeNode);
                
                int count = sscanf(optarg, "%d", &HIOptions.node_id);
                if (count == 0) fatal("option -n/--node requires a numeric argument");
                
                break;
                
            case 'c':
            {
                set_mode(HIModeShowCNID);
                
                int count = sscanf(optarg, "%d", &HIOptions.cnid);
                if (count == 0) fatal("option -c/--cnid requires a numeric argument");
                
                break;
            }
                
            case 's': set_mode(HIModeShowSummary); break;
                
            default: debug("Unhandled argument '%c' (default case).", opt); usage(1); break;
        };
    };
    
    // Quick Exits
    // These are run first because they don't need us to go through the
    // whole disk loading nonsense and should just print and exit.
    
    // Show Help - exits
    if (check_mode(HIModeShowHelp)) usage(0);
    
    // Show Version - exits
    if (check_mode(HIModeShowVersion)) show_version(basename(argv[0]));
    
#pragma mark Prepare Input and Outputs
    
    // If no device path was given, use the volume of the CWD.
    if ( EMPTY_STRING(HIOptions.device_path) ) {
        info("No device specified. Trying to use the root device.");
        HIOptions.device_path = deviceAtPath(".");
        if ( EMPTY_STRING(HIOptions.device_path) ) {
            error("Device could not be determined. Specify manually with -d/--device.");
            exit(1);
        }
        debug("Found device %s", HIOptions.device_path);
    }
    
    // Load the device
OPEN:
    if (hfs_open(&HIOptions.hfs, HIOptions.device_path) == -1) {
        
        if (errno == EBUSY) {
            // If the device is busy, see if we can use the raw disk instead (and aren't already).
            if (strstr(HIOptions.device_path, "/dev/disk") != NULL) {
                char* newDevicePath;
                INIT_STRING(newDevicePath, PATH_MAX + 1);
                strlcat(newDevicePath, "/dev/rdisk", PATH_MAX);
                strlcat(newDevicePath, &HIOptions.device_path[9], PATH_MAX);
                info("Device %s busy. Trying the raw disk instead: %s", HIOptions.device_path, newDevicePath);
                FREE_BUFFER(HIOptions.device_path);
                HIOptions.device_path = newDevicePath;
                goto OPEN;
            }
            exit(1);
            
        } else {
            error("could not open %s", HIOptions.device_path);
            perror("hfs_open");
            exit(errno);
        }
    }
    
    uid_t uid = 99;
    gid_t gid = 99;
    
    // If extracting, determine the UID to become by checking the owner of the output directory (so we can create any requested files later).
    if (check_mode(HIModeExtractFile)) {
        char* dir = strdup(dirname(HIOptions.extract_path));
        if ( !strlen(dir) ) {
            error("Output file directory does not exist: %s", dir);
            exit(1);
        }
        
        struct stat dirstat = {0};
        int result = stat(dir, &dirstat);
        if (result == -1) {
            perror("stat");
            exit(1);
        }
        
        uid = dirstat.st_uid;
        gid = dirstat.st_gid;
    }

#pragma mark Drop Permissions
    
    // If we're root, drop down.
    if (getuid() == 0) {
        debug("Dropping privs");
        if (setgid(gid) != 0) critical("Failed to drop group privs.");
        if (setuid(uid) != 0) critical("Failed to drop user privs.");
        info("Was running as root.  Now running as %u/%u.", getuid(), getgid());
    }
    
#pragma mark Load Initial Data
    
    if (hfs_load(&HIOptions.hfs) == -1) {
        perror("hfs_load");
        exit(errno);
    }
    
    set_hfs_volume(&HIOptions.hfs); // Set the context for certain hfs_pstruct.h calls.
    
    // Load the tree
    if (HIOptions.tree_type != NULL) {
        if (HIOptions.tree_type == BTreeOptionCatalog) // Yes, comparing pointers. That's why it's a constant.
            HIOptions.tree = hfs_get_catalog_btree(&HIOptions.hfs);
        else if (HIOptions.tree_type == BTreeOptionExtents)
            HIOptions.tree = hfs_get_extents_btree(&HIOptions.hfs);
        else if (HIOptions.tree_type == BTreeOptionAttributes)
            HIOptions.tree = hfs_get_attribute_btree(&HIOptions.hfs);
        else if (HIOptions.tree_type == BTreeOptionHotfile) {
            HFSBTreeNode node;
            hfs_record_id recordID;
            hfs_node_id parentfolder = kHFSRootFolderID;
            HFSUniStr255 name = wcstohfsuc(L".hotfiles.btree");
            int found = hfs_catalog_find_record(&node, &recordID, &HIOptions.hfs, parentfolder, name);
            if (found != 1) {
                error("Hotfiles btree not found. Target must be a hard drive with a copy of Mac OS X that has successfully booted to create this file (it will not be present on SSDs).");
                exit(1);
            }
            HFSPlusCatalogRecord* record = (HFSPlusCatalogRecord*)node.records[recordID].value;
            HFSFork fork = hfsfork_make(&HIOptions.hfs, record->catalogFile.dataFork, 0x00, record->catalogFile.fileID);
            
            int result = hfs_btree_init(&HIOptions.tree, &fork);
            if (result < 1) {
                error("Error initializing hotfiles btree.");
                exit(1);
            }
            
        } else
            critical("Unsupported tree: %s", HIOptions.tree_type);
        
    } else {
        // Default to catalog lookups.
        HIOptions.tree = hfs_get_catalog_btree(&HIOptions.hfs);
    }
    
#pragma mark Process Requests
    
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
                if (!success) critical("Could not get the journal info block!");
                PrintJournalInfoBlock(&block);
            } else {
                warning("Consistency error: volume attributes indicate it is journaled but the journal info block is empty!");
            }
        }
    }
    
    // Show a path's series of records
    if (check_mode(HIModeShowPathInfo)) {
        debug("Show path info.");
        showPathInfo();
    }
    
    // Show B-Tree info
    if (check_mode(HIModeShowBTreeInfo)) {
        debug("Printing tree info.");
        if (HIOptions.tree.fork.cnid == kHFSCatalogFileID) {
            PrintHeaderString("Catalog B-Tree Header");
        } else if (HIOptions.tree.fork.cnid == kHFSExtentsFileID) {
            PrintHeaderString("Extents B-Tree Header");
        } else if (HIOptions.tree.fork.cnid == kHFSAttributesFileID) {
            PrintHeaderString("Attributes B-Tree Header");
        } else if (HIOptions.tree_type == BTreeOptionHotfile) {
            PrintHeaderString("Hotfile B-Tree Header");
        } else {
            critical("Unknown tree type: %s", HIOptions.tree_type);
            return 1;
        }
        
        PrintTreeNode(&HIOptions.tree, 0);
    }
    
    // Show a B-Tree node by ID
    if (check_mode(HIModeShowBTreeNode)) {
        debug("Printing tree node.");
        if (HIOptions.tree_type == BTreeOptionCatalog) {
            PrintHeaderString("Catalog B-Tree Node %d", HIOptions.node_id);
        } else if (HIOptions.tree_type == BTreeOptionExtents) {
            PrintHeaderString("Extents B-Tree Node %d", HIOptions.node_id);
        } else if (HIOptions.tree_type == BTreeOptionAttributes) {
            PrintHeaderString("Attributes B-Tree Node %d", HIOptions.node_id);
        } else if (HIOptions.tree_type == BTreeOptionHotfile) {
            PrintHeaderString("Hotfile B-Tree Node %d", HIOptions.node_id);
        } else {
            critical("Unknown tree type: %s", HIOptions.tree_type);
            return 1;
        }
        
        PrintTreeNode(&HIOptions.tree, HIOptions.node_id);
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
    
    // Extract any found files (not complete; FIXME: dropping perms breaks right now)
    if (check_mode(HIModeExtractFile)) {
        if (HIOptions.extract_HFSFork.cnid == 0 && HIOptions.tree.fork.cnid != 0) {
            HIOptions.extract_HFSFork = HIOptions.tree.fork;
        }
        
        // Extract any found data, if requested.
        if (HIOptions.extract_path != NULL && strlen(HIOptions.extract_path) != 0) {
            debug("Extracting data.");
            if (HIOptions.extract_HFSPlusCatalogFile.fileID != 0) {
                extractHFSPlusCatalogFile(&HIOptions.hfs, &HIOptions.extract_HFSPlusCatalogFile, HIOptions.extract_path);
            } else if (HIOptions.extract_HFSFork.cnid != 0) {
                extractFork(&HIOptions.extract_HFSFork, HIOptions.extract_path);
            }
        }
    }
    
    // Clean up
    hfs_close(&HIOptions.hfs);
    
    debug("Done.");
    
    return 0;
}

