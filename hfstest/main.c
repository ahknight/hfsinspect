//
//  main.c
//  hfstest
//
//  Created by Adam Knight on 5/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <stdlib.h> //ANSI C
#include <stdio.h>  //ANSI C I/O
#include <unistd.h> //POSIX API

#include <sys/errno.h>

#include <locale.h>
#include <string.h>

#include "hfs.h"
#include "hfs_pstruct.h"


int     main                        (int argc, char **argv);
ssize_t extractFork                 (const HFSFork* fork, const char* path);
void    extractHFSPlusCatalogFile   (const HFSVolume *hfs, const HFSPlusCatalogFile* file, const char* extractPath);


int main(int argc, char **argv)
{
	char*       path = NULL;
    u_int8_t    showCatalog = 0;
    u_int8_t    showExtents = 0;
    u_int8_t    showVolInfo = 0;
    u_int8_t    dumpCatalog = 0;
    u_int8_t    printRecord = 0;
    u_int32_t   record_parent = 0;
    char*       record_file = NULL;
    u_int32_t   CNID = 0;
    char*       dumpfile = NULL;
    u_int32_t   catalogNodeID = 0;
    u_int32_t   extentsNodeID = 0;
    char*       extractPath = NULL;
    
    bool        showFile = false;
    char*       showFilePath = NULL;
    
    setlocale(LC_ALL, "");
    
    char opt;
    while ( (opt = getopt(argc, argv, "hic:e:d:p:n:f:x:")) != -1 ) {
        switch ( opt ) {
            case 'h':
            {
                error("usage: hfsinspect [-icedp] filesystem");
                break;
            }
            case 'i':
            {
                showVolInfo ^= 1;
                break;
            }
            case 'c':
            {
                showCatalog = 1;
                sscanf(optarg, "%u", &catalogNodeID);
                if (!catalogNodeID) path = optarg;
                break;
            }
            case 'e':
            {
                showExtents = 1;
                sscanf(optarg, "%u", &extentsNodeID);
                if (!extentsNodeID) path = optarg;
                break;
            }
            case 'd':
            {
                dumpCatalog = 1;
                if (optarg != NULL) {
                    dumpfile = malloc(strlen(optarg));
                    strcpy(dumpfile, optarg);
                } else {
                    error("option -d requires a file");
                    exit(1);
                }
                
                break;
            }
            case 'x':
            {
                if (optarg != NULL) {
                    extractPath = malloc(strlen(optarg));
                    strcpy(extractPath, optarg);
                } else {
                    error("option -x requires a local path");
                    exit(1);
                }
                
                break;
            }
            case 'f':
            {
                showFile = 1;
                if (optarg != NULL && strlen(optarg)) {
//                    printf("Len: %zu\n", strlen(optarg));
                    showFilePath = strdup(optarg);
                } else {
                    error("option -f requires a path");
                    exit(1);
                }
                
                break;
            }
            case 'p':
            {
                printRecord = 1;
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
            case 'n':
            {
                if (optarg != NULL) {
                    sscanf(optarg, "%u", &CNID);
                } else {
                    error("option -n requires a numeric CNID");
                    exit(1);
                }
                break;
            }
                
            default:
            {
                // Unknown option.
                exit(1);
                break;
            }
        }
    }
    
    // Take the first non-flag as the path.
    if (argv[optind])
        path = argv[optind];
    
    // Show *something*
    if (!showCatalog && !showExtents && !showVolInfo && !printRecord && !showFile)
        showVolInfo = 1;
	
    // Need a path
    if (path == NULL || strlen(path) == 0) {
        error("no path specified");
        exit(1);
    }
    
    // Check for a filesystem
    HFSVolume hfs = {};
    
    if (-1 == hfs_open(&hfs, path)) {
        error("could not open %s", path);
        perror("hfs_open");
        return errno;
    }
    
    if (-1 == hfs_load(&hfs)) {
        perror("hfs_load");
        return errno;
    }
    
    set_hfs_volume(&hfs); // Set the context for certain hfs_pstruct.h calls.
    
    // Show a file's record
    if (showFile) {
        if (strlen(showFilePath) == 0) {
            error("Path is required.");
            exit(1);
        }
        
        char* file_path = strdup(showFilePath);
        char* segment;
        HFSPlusCatalogFile *fileRecord = NULL;
        u_int32_t parentID = kHFSRootFolderID;
        
        while ( (segment = strsep(&file_path, "/")) != NULL ) {
//            out("Segment: %d:%s", parentID, segment);
            
            HFSPlusCatalogKey key = {};
            const void* value = NULL;
            
            HFSBTreeNode node = {};
            u_int32_t recordID = 0;
            wchar_t* search_string = malloc(256);
            mbstowcs(search_string, segment, 256);
            int8_t result = hfs_catalog_find_record(&node, &recordID, &hfs, parentID, search_string, 0);
            free(search_string);
            
            if (result) {
//                out("\n\nRecord found:");
                PrintNodeRecord(&node, recordID);
                
//                out("%s: parent: %d (node %d; record %d)", segment, parentID, node.nodeNumber, recordID);
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
                critical("File not found.");
                
            }
        
        }
        
        if (fileRecord != NULL && extractPath != NULL) {
            extractHFSPlusCatalogFile(&hfs, fileRecord, extractPath);
        }
        
        free(file_path);
        
    }
    
    // Print catalog record
    if (printRecord) {
        HFSBTreeNode node = {};
        u_int32_t recordID = 0;
        wchar_t* search_string = malloc(256);
        mbstowcs(search_string, record_file, 256);
        int8_t result = hfs_catalog_find_record(&node, &recordID, &hfs, record_parent, search_string, 0);
        free(search_string);
        if (result) {
//            PrintNode(&node);
            out("\nRecord found:");
            PrintNodeRecord(&node, recordID);
        } else {
            error("Record not found.");
        }
        
        if (extractPath != NULL) {
            if (result == kHFSPlusFileRecord) {
                HFSPlusCatalogFile *file = node.records[recordID].value;
                extractHFSPlusCatalogFile(&hfs, file, extractPath);

            } else {
                error("Cannot extract non-files.");
            }
        }
    }
    
    // Dump the catalog to a file
    if (dumpCatalog) {
        if ( dumpfile != NULL && !strlen(dumpfile) ) {
            error("no dump file specified");
            exit(1);
        }
        HFSBTree catalog = hfs_get_catalog_btree(&hfs);
        u_int32_t nodeID = CNID;
        u_int32_t nodeCount = catalog.headerRecord.totalNodes;
        u_int32_t freeCount = catalog.headerRecord.freeNodes;
        u_int32_t errorCount = 0;
        
        FILE *f = fopen(dumpfile, "wb");
        if (f == NULL) {
            perror("fopen");
            exit(1);
        }
        
        // ESC[s saves cursor position; ESC[u restores. ESC[G is Beginning Of Line. The latter seems to be cleaner.
        while (nodeID < nodeCount) {
            if (nodeID % 10 == 0) printf("\x1b[G Dumping node %u", nodeID);
            
            HFSBTreeNode node = {};
            int8_t result = hfs_btree_get_node(&node, &catalog, nodeID);
            if (result == -1) {
                errorCount++;
                error("skipped node: %u is invalid or out of range.", nodeID);
                fseek(f, catalog.headerRecord.nodeSize, SEEK_CUR); // += node size
                if (errorCount > freeCount) {
                    error("*** WARNING: There are more invalid nodes than expected. (free nodes: %u; errors: %u)", freeCount, errorCount);
                }
            } else {
                fwrite(node.buffer.data, node.nodeSize, 1, f);
                fflush(f);
            }
            hfs_btree_free_node(&node);
            
            nodeID++;
            
        }
        
        fclose(f);
        exit(0);
    }
    
    // Show requested data
    if (showVolInfo)
        PrintVolumeHeader(&hfs.vh);
    
    if (showCatalog) {
        printf("\n\n# BEGIN B-TREE: CATALOG\n");
        
        HFSBTree tree = hfs_get_catalog_btree(&hfs);
        
        if (catalogNodeID) {
            PrintTreeNode(&tree, (u_int32_t)catalogNodeID);
        } else {
            PrintTreeNode(&tree, 0);
            PrintTreeNode(&tree, tree.headerRecord.rootNode);
        }
        
        if (extractPath != NULL) {
            extractFork(&tree.fork, extractPath);
        }
    }
    
    if (showExtents) {
        printf("\n\n# BEGIN B-TREE: EXTENTS\n");
        
        HFSBTree tree = hfs_get_extents_btree(&hfs);
        
        if (extentsNodeID) {
            PrintTreeNode(&tree, (u_int32_t)extentsNodeID);
        } else {
            PrintTreeNode(&tree, 0);
            PrintTreeNode(&tree, tree.headerRecord.rootNode);
        }
        
        if (extractPath != NULL) {
            extractFork(&tree.fork, extractPath);
        }
    }
    
    // Clean up
	hfs_close(&hfs);
    
    return 0;
}


ssize_t extractFork(const HFSFork* fork, const char* path)
{
    out("Extracting data fork to %s", path);
    off_t offset = 0;
    size_t chunkSize = fork->hfs.vh.blockSize*100;
    
    Buffer chunk = buffer_alloc(chunkSize);
    
    FILE* f = fopen(path, "w");
    if (f == NULL) {
        perror("open");
        return -1;
    }
    
    while ( offset < fork->forkData.logicalSize ) {
        chunk.offset = 0;
        ssize_t size = hfs_read_fork_range(&chunk, fork, chunkSize, offset);
        if (size < 1) {
            perror("read");
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
        HFSFork fork = make_hfsfork(hfs, file->dataFork, HFSDataForkType, file->fileID);
        ssize_t size = extractFork(&fork, extractPath);
        if (size < 0) {
            perror("extract");
            critical("Extract data fork failed.");
        }
    }
    if (file->resourceFork.logicalSize > 0) {
        char* outputPath = malloc(FILENAME_MAX);
        ssize_t size;
        size = strlcpy(outputPath, extractPath, sizeof(outputPath));
        if (size < 1) critical("Could not create destination filename.");
        size = strlcat(outputPath, ".rsrc", sizeof(outputPath));
        if (size < 1) critical("Could not create destination filename.");
        
        HFSFork fork = make_hfsfork(hfs, file->resourceFork, HFSResourceForkType, file->fileID);
        size = extractFork(&fork, extractPath);
        if (size < 0) {
            perror("extract");
            critical("Extract resource fork failed.");
        }
    }
    // TODO: Merge forks, set attributes ... essentially copy it. Maybe extract in AppleSingle/Double/MacBinary format?
}
