//
//  extract_file.c
//  hfsinspect
//
//  Created by Adam Knight on 4/1/14.
//  Copyright (c) 2014 Adam Knight. All rights reserved.
//

#include "operations.h"
#include <errno.h>              // errno/perror

ssize_t extractFork(const HFSFork* fork, const String extractPath)
{
    FILE*    f_out         = NULL;
    FILE*    f_in          = NULL;
    off_t    offset        = 0;
    size_t   chunkSize     = 0;
    Bytes    chunk         = NULL;
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

    f_in       = fopen_hfsfork((HFSFork*)fork);
    fseeko(f_in, 0, SEEK_SET);

    chunkSize  = fork->hfs->block_size*256;    //1-2MB, generally.
    totalBytes = fork->logicalSize;

    format_size(ctx, totalStr, totalBytes, 100);
    format_size(ctx, bytesStr, bytes, 100);

    ALLOC(chunk, chunkSize);

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

    FREE(chunk);

    fflush(f_out);
    fclose(f_out);
    fclose(f_in);

    Print(ctx, "\nCopy complete.");
    return offset;
}

void extractHFSPlusCatalogFile(const HFS* hfs, const HFSPlusCatalogFile* file, const String extractPath)
{
    if (file->dataFork.logicalSize > 0) {
        HFSFork* fork = NULL;
        if ( hfsfork_make(&fork, hfs, file->dataFork, HFSDataForkType, file->fileID) < 0 ) {
            die(1, "Could not create fork for fileID %u", file->fileID);
        }
        ssize_t  size = extractFork(fork, extractPath);
        if (size < 0) {
            perror("extract");
            die(1, "Extract data fork failed.");
        }
        hfsfork_free(fork);
    }
    if (file->resourceFork.logicalSize > 0) {
        String  outputPath = NULL;
        ALLOC(outputPath, FILENAME_MAX);
        ssize_t size;
        size = strlcpy(outputPath, extractPath, sizeof(outputPath));
        if (size < 1) die(1, "Could not create destination filename.");
        size = strlcat(outputPath, ".rsrc", sizeof(outputPath));
        if (size < 1) die(1, "Could not create destination filename.");

        HFSFork* fork = NULL;
        if ( hfsfork_make(&fork, hfs, file->resourceFork, HFSResourceForkType, file->fileID) < 0 )
            die(1, "Could not create fork for fileID %u", file->fileID);

        size = extractFork(fork, extractPath);
        if (size < 0) die(1, "Extract resource fork failed.");

        hfsfork_free(fork);

        FREE(outputPath);
    }
    // TODO: Merge forks, set attributes ... essentially copy it. Maybe extract in AppleSingle/Double/MacBinary format?
}

