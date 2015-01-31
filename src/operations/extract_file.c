//
//  extract_file.c
//  hfsinspect
//
//  Created by Adam Knight on 4/1/14.
//  Copyright (c) 2014 Adam Knight. All rights reserved.
//

#include <stdlib.h>
#include "operations.h"
#include "hfs/output_hfs.h"
#include "hfs/hfs_io.h"

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
    size_t chunkSize    = fork->hfs->block_size*256; //1-2MB, generally.
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

