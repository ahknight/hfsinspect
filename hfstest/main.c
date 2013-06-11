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
#include "stringtools.h"

int     main                        (int argc, char **argv);
ssize_t extractFork                 (const HFSFork* fork, const char* extractPath);
void    extractHFSPlusCatalogFile   (const HFSVolume *hfs, const HFSPlusCatalogFile* file, const char* extractPath);

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
    
    VolumeSummary summary = {0};
    
    HFSBTree catalog = hfs_get_catalog_btree(hfs);
    u_int32_t cnid = catalog.headerRecord.firstLeafNode;
    
    while (1) {
        HFSBTreeNode node = {};
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
                    if (file->flags & kHFSHasLinkChainMask) { summary.hardLinkFileCount++; }
                    
                    // alias
                    if (file->userInfo.fdType == 'alis') { summary.aliasCount++; }
                    
                    // symlink
                    if (file->userInfo.fdType == 'slnk') { summary.symbolicLinkCount++; }
                    
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

int main(int argc, char **argv)
{
    debug("Hello, and welcome!");
    
	char*       path = NULL;
    bool        showCatalog = false;
    bool        showExtents = false;
    bool        showVolInfo = false;
    bool        dumpBTree = false;
    bool        printRecord = false;
    u_int32_t   record_parent = 0;
    char*       record_file = NULL;
    u_int32_t   nodeID = 0;
    char*       dumpfile = NULL;
    char*       extractPath = NULL;
    
    bool        showFile = false;
    char*       showFilePath = NULL;
    
    bool        summary = false;
    
    setlocale(LC_ALL, "");
    
    char opt;
    while ( (opt = getopt(argc, argv, "hiced:p:n:f:x:s")) != -1 ) {
        switch ( opt ) {
            case 'h':
            {
                error("usage: hfsinspect [-icedp] filesystem");
                break;
            }
            case 's':
            {
                summary = true;
                break;
            }
            case 'i':
            {
                showVolInfo = true;
                break;
            }
            case 'c':
            {
                showCatalog = true;
                break;
            }
            case 'e':
            {
                showExtents = true;
                break;
            }
            case 'd':
            {
                dumpBTree = true;
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
                showFile = true;
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
                printRecord = true;
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
                    sscanf(optarg, "%u", &nodeID);
                } else {
                    error("option -n requires a numeric node ID");
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
//    if (!showCatalog && !showExtents && !showVolInfo && !printRecord && !showFile)
//        showVolInfo = 1;
	
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
        exit(errno);
    }
    
    // If we're root, drop down.
    if (getuid() == 0) {
        if (setgid(99) != 0)
            critical("Failed to drop group privs.");
        if (setuid(99) != 0)
            critical("Failed to drop user privs.");
        info("Was running as root.  Now running as %u/%u.", getuid(), getgid());
    }
    
    if (-1 == hfs_load(&hfs)) {
        perror("hfs_load");
        exit(errno);
    }
    
    set_hfs_volume(&hfs); // Set the context for certain hfs_pstruct.h calls.
    
    
    if (summary) {
        VolumeSummary summary = generateVolumeSummary(&hfs);
        PrintVolumeSummary(&summary);
    }
    
    
    
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
    if (dumpBTree) {
        if ( dumpfile != NULL && !strlen(dumpfile) ) {
            error("no dump file specified");
            exit(1);
        }
        HFSBTree catalog = hfs_get_catalog_btree(&hfs);
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
        
//        HFSBTree extents = hfs_get_extents_btree(&hfs);
        HFSBTree catalog = hfs_get_catalog_btree(&hfs);
        
        if (nodeID) {
            PrintTreeNode(&catalog, (u_int32_t)nodeID);
        } else {
            PrintTreeNode(&catalog, 0);
//            PrintTreeNode(&catalog, catalog.headerRecord.rootNode);
//            PrintTreeNode(&extents, 0);
        }
        
        if (extractPath != NULL) {
            extractFork(&catalog.fork, extractPath);
        }
    }
    
    if (showExtents) {
        printf("\n\n# BEGIN B-TREE: EXTENTS\n");
        
        HFSBTree extents = hfs_get_extents_btree(&hfs);
        
        if (nodeID) {
            PrintTreeNode(&extents, (u_int32_t)nodeID);
        } else {
            PrintTreeNode(&extents, 0);
            PrintTreeNode(&extents, extents.headerRecord.rootNode);
        }
        
        if (extractPath != NULL) {
            extractFork(&extents.fork, extractPath);
        }
    }
    
    // Clean up
    hfs_close(&hfs);
    
    return 0;
}


ssize_t extractFork(const HFSFork* fork, const char* extractPath)
{
    out("Extracting data fork to %s", extractPath);
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
