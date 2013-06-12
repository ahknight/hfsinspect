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
#include <malloc/malloc.h>

#include "hfs.h"
#include "stringtools.h"

void    usage                       (int status);
int     main                        (int argc, char* const *argv);
int     compare_ranked_files        (const Rank * a, const Rank * b);
VolumeSummary generateVolumeSummary (HFSVolume* hfs);
ssize_t extractFork                 (const HFSFork* fork, const char* extractPath);
void    extractHFSPlusCatalogFile   (const HFSVolume *hfs, const HFSPlusCatalogFile* file, const char* extractPath);
char*   deviceAtPath                (char*);
bool    resolveDeviceAndPath        (char* path_in, char* device_out, char* path_out);

char* BTreeOptionCatalog    = "catalog";
char* BTreeOptionExtents    = "extents";
char* BTreeOptionAttributes = "attributes";
char* BTreeOptionHotfile    = "hotfile";

void usage(int status)
{
    /* hfsdebug-lite args remaining...
     -0,        --freespace     display all free extents on the volume
     -c CNID,   --cnid=CNID     specify an HFS+ catalog node ID
     -e,        --examples      display some usage examples
     -f,        --fragmentation display all fragmented files on the volume
     -H,        --hotfiles      display the hottest files on the volume; requires
     the -t (--top=TOP) option for the number to list
     -j,        --journalinfo   display information about the volume's journal
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
     -V VOLUME, --volume=VOLUME specify an HFS+ volume, where VOLUME is its mount
             point. VOLUME may also be the path of any file
             within the volume
     -x FILTERDYLIB, --filter=FILTERDYLIB
             run the filter implemented in the dynamic library
             whose path is FILTERDYLIB. Alternatively,
             FILTERDYLIB can be 'builtin:<name>', where <name>
             specifies one of the built-in filters: atime,
             crtime, dirhardlink, hardlink, mtime, and sxid
     -X FILTERARGS, --filter_args=FILTERARGS
             pass this argument string to the filter callback
     */
    char* help = "hfsinspect [hd:vn:b:o:sp:F:] \n\
SOURCES: \n\
    hfsinspect will use the root filesystem by default, or the filesystem containing a target file in some cases. If you wish to\n\
    specify a specific device or volume, you can with the following options:\n\
    -d DEV,     --device DEV    Path to device or file containing a bare HFS+ filesystem (no partition map or HFS wrapper) \n\
    -V VOLUME   --volume VOLUME Use the path to a mounted disk or any file on the disk to use a mounted volume. \n\
\n\
INFO: \n\
    By default, hfsinspect will just show you the volume header and quit.  Use the following options to get more specific data.\n\
    -h,         --help          Show help and quit. \n\
                --version       Show version information and quit. \n\
    -b NAME,    --btree NAME    Specify which HFS+ B-Tree to work with. Supported options: attributes, catalog, extents, or hotfile. \n\
    -n,         --node ID       Dump an HFS+ B-Tree node by ID (must specify tree with -b). \n\
    -v,         --volumeheader  Dump the volume header. \n\
\n\
OUTPUT: \n\
    You can optionally have hfsinspect dump any fork it finds as the result of an operation. This includes B-Trees or file forks.\n\
    Use a command like \"-b catalog -o catalog.dump\" to extract the catalog file from the boot drive, for instance.\n\
    -o PATH,    --output PATH   Use with -b or -p to dump a raw data fork (when used with -b, will dump the HFS+ tree file). \n\
";
    fprintf(stderr, "%s", help);
    exit(status);
}

int main (int argc, char* const *argv)
{
    
    if (argc == 1) usage(1);
    
    setlocale(LC_ALL, "");
    
    int         show_version        = false;
    int         show_volume_header  = false;
    int         summary             = false;
    
    char*       devicePath          = NULL;
    char*       filePath            = NULL;
    
    char*       btree               = NULL;
    long        node_id             = -1;
    
    u_int32_t   record_parent       = 0;
    char*       record_file         = NULL;
    
    char*       output              = NULL;
    
    
    // If anything sets these and output is a valid path then we will extract the source to the output path.
    HFSPlusCatalogFile  extractFile;
    HFSFork             extractHFSFork;
    
    /* options descriptor */
    struct option longopts[] = {
        { "help",           no_argument,            NULL,                   'h' },
        { "version",        no_argument,            &show_version,          1   },
        { "device",         required_argument,      NULL,                   'd' },
        { "volumeheader",   no_argument,            &show_volume_header,    1   },
        { "node",           required_argument,      NULL,                   'n' },
        { "btree",          required_argument,      NULL,                   'b' },
        { "output",         required_argument,      NULL,                   'o' },
        { "summary",        no_argument,            &summary,               's' },
        { "path",           required_argument,      NULL,                   'p' },
        { "path_abs",       required_argument,      NULL,                   'P' },
        { NULL,             0,                      NULL,                   0   }
    };
    
    /* short options */
    char* shortopts = "hd:vn:b:o:sp:P:F:";
    
    char opt;
    while ((opt = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (opt) {
            case 'h':
            {
                debug("Showing help.\n");
                usage(0);
                break;
            }
                
            case 0:
            {
                if (show_version) {
                    debug("Showing version.\n");
                    fprintf(stdout, "%s %s\n", basename(argv[0]), "1.0");
                    exit(0);
                }
                break;
            }
                
            case 'd':
            {
                if (optarg != NULL) {
                    debug("Setting path.\n");
                    if (devicePath != NULL && malloc_size(devicePath)) free(devicePath);
                    size_t length = strlen(optarg);
                    devicePath = malloc(length);
                    strncpy(devicePath, optarg, length);
                } else {
                    error("option -d/--device requires a path argument\n");
                    exit(1);
                }
                debug("Using device path %s\n", devicePath);
                break;
            }
                
            case 'v':
            {
                debug("Showing volume header.\n");
                show_volume_header = true;
                break;
            }
                
            case 'p':
            {
                if (optarg != NULL) {
                    debug("Setting file path.");
                    if (devicePath && malloc_size(devicePath)) free(devicePath);
                    if (filePath && malloc_size(filePath)) free(filePath);
                    
                    devicePath = malloc(PATH_MAX); memset(devicePath, '\0', PATH_MAX);
                    filePath = malloc(PATH_MAX); memset(devicePath, '\0', PATH_MAX);
                    bool success = resolveDeviceAndPath(optarg, devicePath, filePath);
                    
                    if (!success || strlen(devicePath) == 0) {
                        error("Device could not be determined. Specify manually with -d/--device.");
                        exit(1);
                    }
                    info("Device path set to %s", devicePath);
                    
                    if (strlen(filePath) == 0) {
                        error("Path must be an absolute path from the root of the target filesystem.");
                        exit(1);
                    }
                    
                    info("Resolved file path %s to %s", optarg, filePath);
                    
                } else {
                    error("option -p/--path requires a path argument\n");
                    exit(1);
                }
                
                break;
            }

            case 'P':
            {
                if (optarg != NULL) {
                    debug("Setting file path.");
                    if (filePath && strlen(filePath)) free(filePath);
                    size_t length = strlen(optarg);
                    filePath = malloc(length);
                    strncpy(filePath, optarg, length);
                } else {
                    error("option -p/--path requires a path argument\n");
                    exit(1);
                }
                
                if (filePath[0] != '/') {
                    error("Path must be an absolute path from the root of the target filesystem.");
                    exit(1);
                }
                
                break;
            }

            case 'b':
            {
                if (optarg != NULL) {
                    debug("Setting b-tree.\n");
                    if (btree != NULL && malloc_size(btree)) free(btree);
                    size_t length = strlen(optarg)+1;
                    btree = malloc(length);
                    strlcpy(btree, optarg, length);
                } else {
                    error("option -b/--btree requires a string argument\n");
                    exit(1);
                }
                
                if (
                    (strcmp(btree, BTreeOptionAttributes) != 0) &&
                    (strcmp(btree, BTreeOptionCatalog) != 0) &&
                    (strcmp(btree, BTreeOptionExtents) != 0) &&
                    (strcmp(btree, BTreeOptionHotfile) != 0)
                    ) {
                    error("option -b/--btree must be one of: attribute, catalog, extents, or hotfile (not %s).\n", btree);
                    exit(1);
                }
                
                debug("Set btree to %s\n", btree);
                break;
            }
            
            case 'F':
            {
                if (optarg != NULL && strlen(optarg)) {
                    char* option = strdup(optarg);
                    char* parent = strsep(&option, ":");
                    char* filename = strsep(&option, ":");
                    
                    if (parent && strlen(parent)) {
                        sscanf(parent, "%u", &record_parent);
                    }
                    if (filename) {
                        record_file = strdup(filename);
                    }
                    
                    free(option);
                }
                
                if (record_file != NULL && record_parent) {
                    break;
                } else {
                    error("option -p requires a parent ID and file (eg. 2:.journal)");
                    exit(1);
                }
            }

            case 'o':
            {
                if (optarg != NULL) {
                    debug("Setting output path.\n");
                    if (output != NULL && malloc_size(output)) free(output);
                    size_t length = strlen(optarg);
                    output = malloc(length);
                    strncpy(output, optarg, length);
                } else {
                    error("option -o/--output requires a path argument\n");
                    exit(1);
                }
                debug("Using output path %s\n", output);
                break;
            }
                
            case 'n':
            {
                debug("Setting node argument.\n");
                if (optarg != NULL) {
                    sscanf(optarg, "%ld", &node_id);
                }
                if (node_id == -1) {
                    error("option -n/--node requires a numeric argument\n");
                    exit(1);
                }
                debug("Node is %ld\n", node_id);
                break;
            }
                
            case 's':
            {
                debug("Showing summary.\n");
                summary = true;
                break;
            }
                
            default:
            {
                debug("Unhandled argument '%c' (default case).\n", opt);
                usage(1);
                break;
            }
        };
    };
    
    // Need a path
    if (devicePath == NULL || strlen(devicePath) == 0) {
        info("No device specified. Trying to use the root device.");
        devicePath = deviceAtPath("/");
        if (devicePath == NULL || strlen(devicePath) == 0) {
            error("Device could not be determined. Specify manually with -d/--device.");
            exit(1);
        }
        debug("Found device %s", devicePath);
    }
    
    // Check for a filesystem
    HFSVolume hfs;
    
OPEN:
    if (-1 == hfs_open(&hfs, devicePath)) {
        if (errno == EBUSY) {
            if (strstr(devicePath, "/dev/disk") != NULL) {
                char* newDevicePath = malloc(MAXPATHLEN + 1);
                strlcat(newDevicePath, "/dev/rdisk", MAXPATHLEN);
                strlcat(newDevicePath, &devicePath[9], MAXPATHLEN);
                warning("Device busy. Trying the raw disk instead: %s", newDevicePath);
                free(devicePath); devicePath = NULL;
                devicePath = newDevicePath;
                goto OPEN;
            }
            
            exit(1);
        } else {
            error("could not open %s", devicePath);
            perror("hfs_open");
            exit(errno);
        }
    }
    
    // If we're root, drop down.
    if (getuid() == 0) {
        debug("Dropping privs");
        if (setgid(99) != 0) critical("Failed to drop group privs.");
        if (setuid(99) != 0) critical("Failed to drop user privs.");
        info("Was running as root.  Now running as %u/%u.", getuid(), getgid());
    }
    
    if (hfs_load(&hfs) == -1) {
        perror("hfs_load");
        exit(errno);
    }
    
    set_hfs_volume(&hfs); // Set the context for certain hfs_pstruct.h calls.
    
    // Load the tree
    HFSBTree tree;
    
    if (btree) {
        if (strcmp(btree, BTreeOptionCatalog)) {
            tree = hfs_get_catalog_btree(&hfs);
        } else if (strcmp(btree, BTreeOptionExtents)) {
            tree = hfs_get_extents_btree(&hfs);
        } else {
            critical("Unsupported tree.");
        }
    } else {
        tree = hfs_get_catalog_btree(&hfs);
    }
    
    if (summary) {
        debug("Printing summary.");
        VolumeSummary summary = generateVolumeSummary(&hfs);
        PrintVolumeSummary(&summary);
    }
    
    // Show a file's record
    if (filePath != NULL) {
        debug("Finding record for path: %s", filePath);
        
        if (strlen(filePath) == 0) {
            error("Path is required.");
            exit(1);
        }
        
        char* segment;
        HFSPlusCatalogFile *fileRecord = NULL;
        u_int32_t parentID = kHFSRootFolderID;
        
        while ( (segment = strsep(&filePath, "/")) != NULL ) {
            debug("Segment: %d:%s", parentID, segment);
            
            HFSPlusCatalogKey key;
            const void* value = NULL;
            
            HFSBTreeNode node;
            u_int32_t recordID = 0;
            wchar_t* search_string = malloc(256);
            mbstowcs(search_string, segment, 256);
            int8_t result = hfs_catalog_find_record(&node, &recordID, &hfs, parentID, search_string, 0);
            free(search_string);
            
            if (result) {
                debug("\n\nRecord found:");
                PrintNodeRecord(&node, recordID);
                
                debug("%s: parent: %d (node %d; record %d)", segment, parentID, node.nodeNumber, recordID);
                if (strlen(segment) == 0) continue;
                
                int kind = hfs_get_catalog_leaf_record(&key, &value, &node, recordID);
                switch (kind) {
                    case kHFSPlusFolderRecord:
                    {
                        typedef struct HFSPlusCatalogFolder record_type;
                        record_type *record = (record_type*)value;
                        parentID = record->folderID;
                        break;
                    }
                        
                    case kHFSPlusFileRecord:
                    {
                        typedef struct HFSPlusCatalogFile record_type;
                        record_type *record = (record_type*)value;
                        parentID = record->fileID;
                        fileRecord = record;
                        break;
                    }
                        
                    case kHFSPlusFolderThreadRecord:
                    case kHFSPlusFileThreadRecord:
                    {
                        typedef struct HFSPlusCatalogThread record_type;
                        record_type *record = (record_type*)value;
                        parentID = record->parentID;
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
        
        if (fileRecord != NULL) {
            extractFile = *fileRecord;
        }
    }
    
    // Print catalog record
    if (record_file != NULL) {
        debug("Finding catalog record for %d:%s", record_parent, record_file);
        
        HFSBTreeNode node;
        u_int32_t recordID = 0;
        wchar_t* search_string = malloc(256);
        mbstowcs(search_string, record_file, 256);
        int8_t result = hfs_catalog_find_record(&node, &recordID, &hfs, record_parent, search_string, 0);
        free(search_string);
        if (result) {
            info("Record found:");
            PrintNodeRecord(&node, recordID);
        } else {
            error("Record not found.");
        }
        
        if (result == kHFSPlusFileRecord) {
            extractFile = *(HFSPlusCatalogFile*)node.records[recordID].value;
        }
    }
        
    // Show requested data
    if (show_volume_header) {
        debug("Printing volume header.");
        PrintVolumeHeader(&hfs.vh);
    }
    
    if (tree.fork.cnid != 0 && node_id >= 0) {
        debug("Printing tree node.");
        
        if (strcmp(btree, BTreeOptionCatalog) == 0) {
            print("\n\n# BEGIN B-TREE: CATALOG");
        } else if (strcmp(btree, BTreeOptionExtents) == 0) {
            print("\n\n# BEGIN B-TREE: EXTENTS");
        } else {
            print("# BEGIN B-TREE: UNKNOWN");
        }
        
        if (!node_id) node_id = 0;
        PrintTreeNode(&tree, (u_int32_t)node_id);
    }
    
    if (tree.fork.cnid != 0) {
        extractHFSFork = tree.fork;
    }
    
    // Extract any found data, if requested.
    if (output != NULL) {
        char* extractPath = realpath(output, NULL);
        if (extractPath != NULL) {
            debug("Extracting data.");
            if (extractFile.fileID != 0) {
                extractHFSPlusCatalogFile(&hfs, &extractFile, extractPath);
            } else if (extractHFSFork.cnid != 0) {
                extractFork(&extractHFSFork, extractPath);
            }
            
            free(extractPath);
        }
    }

    // Clean up
    hfs_close(&hfs);
    
    debug("Done.");
    
    return 0;
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
            HFSBTreeNodeRecord *record = &node.records[recNum]; summary.recordCount++;
            
            // fileCount, folderCount
            u_int8_t type = hfs_get_catalog_record_type(&node, recNum);
            switch (type) {
                case kHFSPlusFileRecord:
                {
                    summary.fileCount++;
                    
                    HFSPlusCatalogFile *file = ((HFSPlusCatalogFile*)record->value);
                    
                    // hard link
                    if (file->userInfo.fdType == kHardLinkFileType) { summary.hardLinkFileCount++; }
                    
                    // symlink
                    if (file->userInfo.fdType == kSymLinkFileType) { summary.symbolicLinkCount++; }
                    
                    // alias
                    if (file->userInfo.fdType == 'alis') { summary.aliasCount++; }
                    
                    if (file->dataFork.logicalSize == 0 && file->resourceFork.logicalSize == 0) {
                        summary.emptyFileCount++;
                        
                    } else {
                        if (file->dataFork.logicalSize) {
                            summary.dataFork.count++;
                            
                            summary.dataFork.blockCount      += file->dataFork.totalBlocks;
                            summary.dataFork.logicalSpace    += file->dataFork.logicalSize;
                            
                            if (file->dataFork.extents[1].blockCount > 0) {
                                summary.dataFork.fragmentedCount++;
                                
                                // TODO: Find all extent records
                            }
                        }
                        
                        if (file->resourceFork.logicalSize) {
                            summary.resourceFork.count++;
                            
                            summary.resourceFork.blockCount      += file->resourceFork.totalBlocks;
                            summary.resourceFork.logicalSpace    += file->resourceFork.logicalSize;
                            
                            if (file->resourceFork.extents[1].blockCount > 0) {
                                summary.resourceFork.fragmentedCount++;
                                
                                // TODO: Find all extent records
                            }
                        }
                        
                        size_t fileSize = file->dataFork.logicalSize + file->resourceFork.logicalSize;
                        
                        if (summary.largestFiles[0].measure < fileSize) {
                            summary.largestFiles[0].measure = fileSize;
                            summary.largestFiles[0].cnid = cnid;
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
        if (cnid == 0) { error("out of nodes"); break; } // End Of Line
        
        static int count = 0; count++;
        
        if ((count % 100) == 0) {
            // Update
            size_t space = summary.dataFork.logicalSpace + summary.resourceFork.logicalSpace;
            char* size = sizeString(space);
            
            fprintf(stdout, "\x1b[G%d/%d (files: %llu; directories: %llu; size: %s)",
                    count,
                    catalog.headerRecord.leafRecords,
                    summary.fileCount,
                    summary.folderCount,
                    size
                    );
            fflush(stdout);
            free(size);
        }
        
        //        if (count >= 10000) break;
        
    }
    
    return summary;
}

ssize_t extractFork(const HFSFork* fork, const char* extractPath)
{
    print("Extracting data fork to %s", extractPath);
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
            fwrite(chunk.data, size, 1, f);
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
        char* outputPath = malloc(FILENAME_MAX);
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
        free(outputPath);
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
    
    char* devicePath = malloc(MAXPATHLEN+1);
    strlcpy(devicePath, stats.f_mntfromname, MAXPATHLEN);
    return devicePath;
}

bool resolveDeviceAndPath(char* path_in, char* device_out, char* path_out)
{
    char* path = realpath(path_in, NULL);
    if (path == NULL) {
        perror("realpath");
        critical("Target not found: %s", path_in);
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
