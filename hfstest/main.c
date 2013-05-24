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
#include <hfs/hfs_format.h>

#include <locale.h>
#include <string.h>

#include "hfs.h"
#include "hfs_pstruct.h"


int main(int argc, char **argv)
{
	char*       path;
    u_int8_t    showCatalog = 0;
    u_int8_t    showExtents = 0;
    u_int8_t    showVolInfo = 0;
    u_int8_t    dumpCatalog = 0;
    char        *dumpfile = NULL;
    u_int32_t   catalogNodeID = 0;
    u_int32_t   extentsNodeID = 0;
    
    setlocale(LC_ALL, "");
    
    char opt;
    while ( (opt = getopt(argc, argv, "hic:e:d:")) != -1 ) {
        switch ( opt ) {
            case 'h':
            {
                error("usage: hfsinspect [-iced] filesystem\n");
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
                    error("error: option -d requires a file\n");
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
    if (!showCatalog && !showExtents && !showVolInfo)
        showVolInfo = 1;
	
    // Need a path
    if (path == NULL || strlen(path) == 0) {
        error("error: no path specified\n");
        exit(1);
    }
    
    // Check for a filesystem
    HFSVolume hfs = {};
    
    if (-1 == hfs_open(&hfs, path)) {
        error("error: could not open %s\n", path);
        perror("hfs_open");
        return errno;
    }
    
    if (-1 == hfs_load(&hfs)) {
        perror("hfs_load");
        return errno;
    }
    
    // Dump the catalog to a file
    if (dumpCatalog) {
        if ( dumpfile != NULL && !strlen(dumpfile) ) {
            error("error: no dump file specified\n");
            exit(1);
        }
        HFSBTree catalog = hfs_get_catalog_btree(&hfs);
        u_int32_t nodeID = 215150;
        u_int32_t nodeCount = catalog.headerRecord.totalNodes;
        u_int32_t freeCount = catalog.headerRecord.freeNodes;
        u_int32_t errorCount = 0;
        
        FILE *f = fopen(dumpfile, "wb");
        if (f == NULL) {
            perror("fopen");
            exit(1);
        }
        
        while (nodeID < nodeCount) {
            out("Dumping node %d\010", nodeID);
            
            HFSBTreeNode node = {};
            int8_t result = hfs_btree_get_node(&node, &catalog, nodeID);
            if (result == -1) {
                errorCount++;
                error("skipped node: %d is invalid or out of range.\n", nodeID);
                if (errorCount > freeCount) {
                    error("*** WARNING: There are more invalid nodes than expected. (free nodes: %d; errors: %d)\n", freeCount, errorCount);
                }
            } else {
                fwrite(node.buffer.data, node.blockSize, 1, f);
                fflush(f);
            }
            
            nodeID++;
            
        }
        
        fclose(f);
        exit(0);
    }
    
    // Show requested data
    if (showVolInfo)
        PrintVolumeHeader(&hfs.vh);
    
    if (showCatalog) {
        out("\n\n# BEGIN B-TREE: CATALOG\n");
        
        HFSBTree tree = hfs_get_catalog_btree(&hfs);
        
        if (catalogNodeID) {
            PrintTreeNode(&tree, (u_int32_t)catalogNodeID);
        } else {
            PrintTreeNode(&tree, 0);
            PrintTreeNode(&tree, tree.headerRecord.rootNode);
        }
    }
    
    if (showExtents) {
        out("\n\n# BEGIN B-TREE: EXTENTS\n");
        
        HFSBTree tree = hfs_get_extents_btree(&hfs);
        
        if (extentsNodeID) {
            PrintTreeNode(&tree, (u_int32_t)extentsNodeID);
        } else {
            PrintTreeNode(&tree, 0);
            PrintTreeNode(&tree, tree.headerRecord.rootNode);
        }
    }
    
    // Clean up
	hfs_close(&hfs);
    
    return 0;
}
