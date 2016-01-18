//
//  extract_file.c
//  hfsinspect
//
//  Created by Adam Knight on 4/1/14.
//  Copyright (c) 2014 Adam Knight. All rights reserved.
//

#include "operations.h"

ssize_t extractFork(const HFSPlusFork* fork, const char* extractPath)
{
    FILE*    f_out         = NULL;
    FILE*    f_in          = NULL;
    off_t    offset        = 0;
    size_t   chunkSize     = 0;
    void*    chunk         = NULL;
    ssize_t  nbytes        = 0;
    size_t   totalBytes    = 0;
    size_t   bytes         = 0;
    char     totalStr[100] = {0};
    char     bytesStr[100] = {0};

    out_ctx* ctx           = fork->hfs->vol->ctx;

    set_hfs_volume(fork->hfs);

    Print(ctx, "Extracting CNID %u to %s", fork->cnid, extractPath);
    PrintHFSPlusForkData(ctx, &fork->forkData, fork->cnid, fork->forkType);

    // Open output stream
    f_out = fopen(extractPath, "w");
    if (f_out == NULL) {
        die(1, "could not open %s", extractPath);
    }

    f_in       = fopen_hfsfork((HFSPlusFork*)fork);
    fseeko(f_in, 0, SEEK_SET);

    chunkSize  = fork->hfs->block_size*256;    //1-2MB, generally.
    totalBytes = fork->logicalSize;

    format_size(ctx, totalStr, totalBytes, 100);
    format_size(ctx, bytesStr, bytes, 100);

    SALLOC(chunk, chunkSize);

    do {
        if ( (nbytes = fread(chunk, 1, chunkSize, f_in)) > 0) {
            bytes += fwrite(chunk, 1, nbytes, f_out);
            format_size(ctx, bytesStr, bytes, 100);
            fprintf(stdout, "\rCopying CNID %u to %s: %s of %s copied                ",
                    fork->cnid,
                    extractPath,
                    bytesStr,
                    totalStr);
            fflush(stdout);
        }
    } while (nbytes > 0);

    SFREE(chunk);

    fflush(f_out);
    fclose(f_out);
    fclose(f_in);

    Print(ctx, "\nCopy complete.");
    return offset;
}

void extractHFSPlusCatalogFile(const HFSPlus* hfs, const HFSPlusCatalogFile* file, const char* extractPath)
{
    if (file->dataFork.logicalSize > 0) {
        HFSPlusFork* fork = NULL;
        if ( hfsfork_make(&fork, hfs, file->dataFork, HFSDataForkType, file->fileID) < 0 ) {
            die(1, "Could not create fork for fileID %u", file->fileID);
        }
        ssize_t      size = extractFork(fork, extractPath);
        if (size < 0) {
            perror("extract");
            die(1, "Extract data fork failed.");
        }
        hfsfork_free(fork);
    }
    if (file->resourceFork.logicalSize > 0) {
        char*   outputPath = NULL;
        SALLOC(outputPath, FILENAME_MAX);
        ssize_t size;
        size = strlcpy(outputPath, extractPath, FILENAME_MAX);
        if (size < 1) die(1, "Could not create destination filename.");
        size = strlcat(outputPath, ".rsrc", FILENAME_MAX);
        if (size < 1) die(1, "Could not create destination filename.");

        HFSPlusFork* fork = NULL;
        if ( hfsfork_make(&fork, hfs, file->resourceFork, HFSResourceForkType, file->fileID) < 0 )
            die(1, "Could not create fork for fileID %u", file->fileID);

        size = extractFork(fork, extractPath);
        if (size < 0) die(1, "Extract resource fork failed.");

        hfsfork_free(fork);

        SFREE(outputPath);
    }
    // TODO: Merge forks, set attributes ... essentially copy it. Maybe extract in AppleSingle/Double/MacBinary format?
}

