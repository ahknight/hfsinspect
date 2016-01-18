//
//  hfs_io.c
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs/hfs_io.h"

#include "hfs/range.h"
#include "hfs/extents.h"
#include "hfs/output_hfs.h"
#include "logging/logging.h"    // console printing routines


#define ASSERT_PTR(st) if (st == NULL) { errno = EINVAL; return -1; }

hfs_forktype_t HFSDataForkType     = 0x00;
hfs_forktype_t HFSResourceForkType = 0xFF;

#pragma mark HFS Volume

ssize_t hfs_read(void* buffer, const HFSPlus* hfs, size_t size, size_t offset)
{
    trace("buffer (%p), hfs (%p), size %zu, offset %zu", buffer, hfs, size, offset);

    ASSERT_PTR(buffer);
    ASSERT_PTR(hfs);

    return vol_read(hfs->vol, buffer, size, offset);
}

// Block arguments are relative to the volume.
ssize_t hfs_read_blocks(void* buffer, const HFSPlus* hfs, size_t block_count, size_t start_block)
{
    ASSERT_PTR(buffer);
    ASSERT_PTR(hfs);

    return vol_read(hfs->vol, buffer, block_count * hfs->block_size, start_block * hfs->block_size);
}

#pragma mark funopen - HFSVolume

typedef struct HFSVolumeCookie {
    off_t    cursor;
    HFSPlus* hfs;
} HFSVolumeCookie;

#if defined (BSD)
int hfs_readfn(void* c, char* buf, int nbytes)
#else
ssize_t hfs_readfn(void* c, char* buf, size_t nbytes)
#endif
{
    HFSVolumeCookie* cookie = (HFSVolumeCookie*)c;
    off_t            offset = cookie->cursor;
    ssize_t          bytes  = 0;

    bytes = hfs_read(buf, cookie->hfs, nbytes, offset);
    if (bytes > 0) cookie->cursor += bytes;

    return bytes;
}

#if defined (BSD)
fpos_t hfs_seekfn(void* c, fpos_t pos, int mode)
{
#else
int hfs_seekfn(void* c, off_t* p, int mode)
{
    off_t            pos    = *p;
#endif
    HFSVolumeCookie* cookie = (HFSVolumeCookie*)c;

    switch (mode) {
        case SEEK_CUR:
        {
            pos += cookie->cursor;
            break;
        }

        case SEEK_END:
        {
            pos = (cookie->hfs->vol->length - pos);

        }

        default:
        {
            break;
        }
    }
    cookie->cursor = pos;
#if defined (BSD)
    return pos;
#else
    *p             = pos;
    return 0;
#endif
}

int hfs_closefn(void* c)
{
    HFSVolumeCookie* cookie = (HFSVolumeCookie*)c;
    SFREE(cookie->hfs);
    SFREE(cookie);
    return 0;
}

FILE* fopen_hfs(HFSPlus* hfs)
{
    HFSVolumeCookie* cookie = NULL;
    SALLOC(cookie, sizeof(HFSVolumeCookie));
    SALLOC(cookie->hfs, sizeof(HFSPlus));
    *cookie->hfs = *hfs; // copy

#if defined(BSD)
    return funopen(cookie, hfs_readfn, NULL, hfs_seekfn, hfs_closefn);
#else

    /*
       FILE *fopencookie(void *cookie, const char *mode, cookie_io_functions_t io_funcs);
       struct cookie_io_functions_t {
         cookie_read_function_t  *read;
         cookie_write_function_t *write;
         cookie_seek_function_t  *seek;
         cookie_close_function_t *close;
       };
     */
    cookie_io_functions_t fc_hfs_funcs = {
        hfs_readfn,
        NULL,
        hfs_seekfn,
        hfs_closefn
    };
    return fopencookie(cookie, "r", fc_hfs_funcs);
#endif
}

#pragma mark HFS Fork

int hfsplus_get_special_fork(HFSPlusFork** fork, const HFSPlus* hfs, bt_nodeid_t cnid)
{
    ASSERT_PTR(fork);
    ASSERT_PTR(hfs);

    HFSPlusForkData forkData;

    switch (cnid) {
        case kHFSExtentsFileID:
        {
            //kHFSExtentsFileID		= 3,	/* File ID of the extents file */
            forkData = hfs->vh.extentsFile;
            break;
        }

        case kHFSCatalogFileID:
        {
            //kHFSCatalogFileID		= 4,	/* File ID of the catalog file */
            forkData = hfs->vh.catalogFile;
            break;
        }

        case kHFSBadBlockFileID:
        {
            //kHFSBadBlockFileID		= 5,	/* File ID of the bad allocation block file */

            /*
               The bad block file is neither a special file nor a user file; this is merely convention used in the extents overflow file.
               http://dubeiko.com/development/FileSystems/HFSPLUS/tn1150.html#BTrees
             */
            return -1;
        }

        case kHFSAllocationFileID: //6
        {                          //kHFSAllocationFileID		= 6,	/* File ID of the allocation file (HFS Plus only) */
            forkData = hfs->vh.allocationFile;
            break;
        }

        case kHFSStartupFileID: // 7
        {                       //kHFSStartupFileID		= 7,	/* File ID of the startup file (HFS Plus only) */
            forkData = hfs->vh.startupFile;
            break;
        }

        case kHFSAttributesFileID: // 8
        {                          //kHFSAttributesFileID		= 8,	/* File ID of the attribute file (HFS Plus only) */
            forkData = hfs->vh.attributesFile;
            break;
        }

        default:
        {
            return -1;
        }
    }

    if ( hfsfork_make(fork, hfs, forkData, HFSDataForkType, cnid) < 0 ) {
        return -1;
    }

    return 0;
}

int hfsfork_make (HFSPlusFork** fork, const HFSPlus* hfs, const HFSPlusForkData forkData, hfs_forktype_t forkType, bt_nodeid_t cnid)
{
    trace("fork %p; hfs %p; forkData (%u blocks); forkType %02x; cnid %u", fork, hfs, forkData.totalBlocks, forkType, cnid);
    ASSERT_PTR(fork);
    ASSERT_PTR(hfs);

    HFSPlusFork* f = NULL;
    SALLOC(f, sizeof(HFSPlusFork));

    f->hfs         = (HFSPlus*)hfs;
    f->forkData    = forkData;
    f->forkType    = forkType;
    f->cnid        = cnid;
    f->totalBlocks = forkData.totalBlocks;
    f->logicalSize = forkData.logicalSize;

    f->extents     = extentlist_make();
    if ( hfsplus_extents_get_extentlist_for_fork(f->extents, f) == false) {
        error("Failed to get extents for new fork!");
        SFREE(f);
        return -1;
    }

    *fork = f;

    return 0;
}

void hfsfork_free(HFSPlusFork* fork)
{
    extentlist_free(fork->extents);
    SFREE(fork);
}

ssize_t hfs_read_fork(void* buffer, const HFSPlusFork* fork, size_t block_count, size_t start_block)
{
    ASSERT_PTR(buffer);
    ASSERT_PTR(fork);

    int   loopCounter = 0; // Fail-safe.

    // Keep the original request around
    range request     = make_range(start_block, block_count);

    debug2("Reading from CNID %u (%zd, %zd)", fork->cnid, request.start, request.count);

    // Sanity checks
    if (request.count < 1) {
        error("Invalid request size: %zu blocks", request.count);
        return -1;
    }

    if ( request.start > fork->totalBlocks ) {
        error("Request would begin beyond the end of the file (start block: %zu; file size: %u blocks).", request.start, fork->totalBlocks);
        return -1;
    }

    if ( range_max(request) >= fork->totalBlocks ) {
        request.count = fork->totalBlocks - request.start;
        request.count = MAX(request.count, 1);
        debug("Trimmed request to (%zu, %zu) (file only has %d blocks)", request.start, request.count, fork->totalBlocks);
    }

    char*       read_buffer = NULL;
    SALLOC(read_buffer, block_count * fork->hfs->block_size);
    ExtentList* extentList  = fork->extents;

    // Keep track of what's left to get
    range       remaining   = request;

    while (remaining.count != 0) {
        range   read_range;
        bool    found  = false;
        ssize_t blocks = 0;

        if (++loopCounter > 2000) {
            Extent* extent = NULL;
            TAILQ_FOREACH(extent, extentList, extents) {
                print("%10zd: %10zd %10zd", extent->logicalStart, extent->startBlock, extent->blockCount);
            }
            PrintExtentList(fork->hfs->vol->ctx, extentList, fork->totalBlocks);
            critical("We're stuck in a read loop: request (%zd, %zd); remaining (%zd, %zd)", request.start, request.count, remaining.start, remaining.count);
        }

        debug2("Remaining: (%zd, %zd)", remaining.start, remaining.count);

        found = extentlist_find(extentList, remaining.start, &read_range.start, &read_range.count);
        if (!found) {
            PrintExtentList(fork->hfs->vol->ctx, extentList, fork->totalBlocks);
            critical("Logical block %zd not found in the extents for CNID %d!", remaining.start, fork->cnid);
        }

        if (read_range.count == 0) {
            warning("About to read a null range! Looking for (%zd, %zd), received (%zd, %zd).", remaining.start, remaining.count, read_range.start, read_range.count);
            continue;
        }

        read_range.count = MIN(read_range.count, request.count);

        blocks           = hfs_read_blocks(read_buffer, fork->hfs, read_range.count, read_range.start);
        if (blocks < 0) {
            SFREE(read_buffer);
            perror("read fork");
            critical("Read error.");
            return -1;
        }

        remaining.count -= MIN((size_t)blocks, remaining.count);
        remaining.start += blocks;

        if (remaining.count == 0) break;
    }

    memcpy(buffer, read_buffer, MIN(block_count, request.count) * fork->hfs->block_size);
    SFREE(read_buffer);

    return request.count;
}

// Grab a specific byte range of a fork.
ssize_t hfs_read_fork_range(void* buffer, const HFSPlusFork* fork, size_t size, size_t offset)
{
    ASSERT_PTR(buffer);
    ASSERT_PTR(fork);

    size_t  start_block = 0;
    size_t  byte_offset = 0;
    size_t  block_count = 0;
    char*   read_buffer = NULL;
    ssize_t read_blocks = 0;

    // Range check.
    if (offset > fork->logicalSize) {
        return 0;
    }

    if ( (offset + size) > fork->logicalSize ) {
        size = fork->logicalSize - offset;
        debug("Adjusted read to (%zu, %zu)", offset, size);
    }

    if (size < 1) {
        return 0;
    }

    // The range starts somewhere in this block.
    start_block = (size_t)(offset / fork->hfs->block_size);

    // Offset of the request within the start block.
    byte_offset = (offset % fork->hfs->block_size);

    // Add a block to the read if the offset is not block-aligned.
    block_count = (size / fork->hfs->block_size) + ( ((offset + size) % fork->hfs->block_size) ? 1 : 0);
    
    // If the read is less than a block, ensure that at least one block is read.
    if (block_count == 0 && size > 0)
        block_count = 1;

    // Use the calculated size instead of the passed size to account for block alignment.
    SALLOC(read_buffer, block_count * fork->hfs->block_size);

    // Fetch the data into a read buffer (it may fail).
    read_blocks = hfs_read_fork(read_buffer, fork, block_count, start_block);

    // On success, append the data to the buffer (consumers: set buffer.offset properly!).
    if (read_blocks) {
        memcpy(buffer, (read_buffer + byte_offset), size);
    }

    // Clean up.
    SFREE(read_buffer);

    // The amount we added to the buffer.
    return size;
}

#pragma mark funopen - HFSPlusFork

typedef struct HFSPlusForkCookie {
    off_t        cursor;
    HFSPlusFork* fork;
} HFSPlusForkCookie;

#if defined (BSD)
int fork_readfn(void* c, char* buf, int nbytes)
#else
ssize_t fork_readfn(void* c, char* buf, size_t nbytes)
#endif
{
    HFSPlusForkCookie* cookie = (HFSPlusForkCookie*)c;
    off_t              offset = cookie->cursor;
    size_t             bytes  = 0;

    bytes = hfs_read_fork_range(buf, cookie->fork, nbytes, offset);
    if (bytes > 0) cookie->cursor += bytes;

    return bytes;
}

#if defined (BSD)
fpos_t fork_seekfn(void* c, fpos_t pos, int mode)
{
#else
int fork_seekfn(void* c, off_t* p, int mode)
{
    off_t              pos    = *p;
#endif
    HFSPlusForkCookie* cookie = (HFSPlusForkCookie*)c;

    switch (mode) {
        case SEEK_CUR:
        {
            pos += cookie->cursor;
            break;
        }

        case SEEK_END:
        {
            pos = (cookie->fork->logicalSize - pos);
            break;
        }

        default:
        {
            break;
        }
    }
    cookie->cursor = pos;

#if defined (BSD)
    return pos;
#else
    *p = pos;
    return 0;
#endif
}

int fork_closefn(void* c)
{
    HFSPlusForkCookie* cookie = (HFSPlusForkCookie*)c;
    SFREE(cookie->fork);
    SFREE(cookie);
    return 0;
}

FILE* fopen_hfsfork(HFSPlusFork* fork)
{
    HFSPlusForkCookie* cookie = NULL;
    SALLOC(cookie, sizeof(HFSPlusForkCookie));
    SALLOC(cookie->fork, sizeof(HFSPlusFork));
    *cookie->fork = *fork; // copy

#if defined(BSD)
    return funopen(cookie, fork_readfn, NULL, fork_seekfn, fork_closefn);
#else

    /*
       FILE *fopencookie(void *cookie, const char *mode, cookie_io_functions_t io_funcs);
       struct cookie_io_functions_t {
       cookie_read_function_t  *read;
       cookie_write_function_t *write;
       cookie_seek_function_t  *seek;
       cookie_close_function_t *close;
       };
     */
    cookie_io_functions_t fc_fork_funcs = {
        fork_readfn,
        NULL,
        fork_seekfn,
        fork_closefn
    };
    return fopencookie(cookie, "r", fc_fork_funcs);
#endif
}

