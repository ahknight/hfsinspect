//
//  hfs_io.c
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs/hfs_io.h"

#include "misc/range.h"
#include "hfs/extents.h"
#include "hfs/output_hfs.h"

#define ASSERT_PTR(st) if (st == NULL) { errno = EINVAL; return -1; }

hfs_forktype_t HFSDataForkType = 0x00;
hfs_forktype_t HFSResourceForkType = 0xFF;

#pragma mark HFS Volume

ssize_t hfs_read(void* buffer, const HFS *hfs, size_t size, size_t offset)
{
    ASSERT_PTR(buffer);
    ASSERT_PTR(hfs);
    
    return vol_read(hfs->vol, buffer, size, offset);
}

// Block arguments are relative to the volume.
ssize_t hfs_read_blocks(void* buffer, const HFS *hfs, size_t block_count, size_t start_block)
{
    ASSERT_PTR(buffer);
    ASSERT_PTR(hfs);
    
    unsigned ratio = hfs->block_size / hfs->vol->sector_size;
    return vol_read_blocks(hfs->vol, buffer, block_count*ratio, start_block*ratio);
}

#pragma mark funopen - HFSVolume

typedef struct HFSVolumeCookie {
    off_t  cursor;
    HFS     *hfs;
} HFSVolumeCookie;

#if defined (BSD)
int hfs_readfn(void * c, char * buf, int nbytes)
#else
ssize_t hfs_readfn(void * c, char * buf, size_t nbytes)
#endif
{
    HFSVolumeCookie *cookie = (HFSVolumeCookie*)c;
    off_t offset = cookie->cursor;
    ssize_t bytes = 0;
    
    bytes = hfs_read(buf, cookie->hfs, nbytes, offset);
    if (bytes > 0) cookie->cursor += bytes;
    
    return bytes;
}

#if defined (BSD)
fpos_t hfs_seekfn(void * c, fpos_t pos, int mode)
{
#else
int hfs_seekfn(void * c, off_t* p, int mode)
{
    off_t pos = *p;
#endif
    HFSVolumeCookie *cookie = (HFSVolumeCookie*)c;
    
    switch (mode) {
        case SEEK_CUR:
            pos += cookie->cursor;
            break;
        case SEEK_END:
            pos = (cookie->hfs->vol->length - pos);
            
        default:
            break;
    }
    cookie->cursor = pos;
#if defined (BSD)
    return pos;
#else
    *p = pos;
    return 0;
#endif
}

int hfs_closefn(void * c)
{
    HFSVolumeCookie *cookie = (HFSVolumeCookie*)c;
    free(cookie->hfs);
    free(cookie);
    return 0;
}

FILE* fopen_hfs(HFS* hfs)
{
    HFSVolumeCookie *cookie = calloc(1, sizeof(HFSVolumeCookie));
    cookie->hfs = calloc(1, sizeof(HFS));
    *cookie->hfs = *hfs;
    
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

int hfsfork_get_special(HFSFork** fork, const HFS *hfs, bt_nodeid_t cnid)
{
    ASSERT_PTR(fork);
    ASSERT_PTR(hfs);
    
    HFSPlusForkData forkData;
    
    switch (cnid) {
        case kHFSExtentsFileID:
            //kHFSExtentsFileID		= 3,	/* File ID of the extents file */
            forkData = hfs->vh.extentsFile;
            break;
            
        case kHFSCatalogFileID:
            //kHFSCatalogFileID		= 4,	/* File ID of the catalog file */
            forkData = hfs->vh.catalogFile;
            break;
            
        case kHFSBadBlockFileID:
            //kHFSBadBlockFileID		= 5,	/* File ID of the bad allocation block file */
            /*
             The bad block file is neither a special file nor a user file; this is merely convention used in the extents overflow file.
             http://dubeiko.com/development/FileSystems/HFSPLUS/tn1150.html#BTrees
             */
            return -1;
            break;
            
        case kHFSAllocationFileID: //6
            //kHFSAllocationFileID		= 6,	/* File ID of the allocation file (HFS Plus only) */
            forkData = hfs->vh.allocationFile;
            break;
            
        case kHFSStartupFileID: // 7
            //kHFSStartupFileID		= 7,	/* File ID of the startup file (HFS Plus only) */
            forkData = hfs->vh.startupFile;
            break;
            
        case kHFSAttributesFileID: // 8
            //kHFSAttributesFileID		= 8,	/* File ID of the attribute file (HFS Plus only) */
            forkData = hfs->vh.attributesFile;
            break;
            
        default:
            return -1;
            break;
    }
    
    if ( hfsfork_make(fork, hfs, forkData, HFSDataForkType, cnid) < 0 ) {
        return -1;
    }
    
    return 0;
}

int hfsfork_make (HFSFork** fork, const HFS *hfs, const HFSPlusForkData forkData, hfs_forktype_t forkType, bt_nodeid_t cnid)
{
    ASSERT_PTR(fork);
    ASSERT_PTR(hfs);
    
    HFSFork *f = NULL;
    ALLOC(f, sizeof(HFSFork));
    
    f->hfs = (HFS*)hfs;
    f->forkData = forkData;
    f->forkType = forkType;
    f->cnid = cnid;
    f->totalBlocks = forkData.totalBlocks;
    f->logicalSize = forkData.logicalSize;
    
    f->extents = extentlist_make();
    if ( hfs_extents_get_extentlist_for_fork(f->extents, f) == false) {
        critical("Failed to get extents for new fork!");
        FREE(f);
        return -1;
    }
    
    *fork = f;
    
    return 0;
}

void hfsfork_free(HFSFork *fork)
{
    extentlist_free(fork->extents);
    FREE(fork);
}

ssize_t hfs_read_fork(void* buffer, const HFSFork *fork, size_t block_count, size_t start_block)
{
    ASSERT_PTR(buffer);
    ASSERT_PTR(fork);
    
    int loopCounter = 0; // Fail-safe.
    
    // Keep the original request around
    range request = make_range(start_block, block_count);
    
//    debug("Reading from CNID %u (%d, %d)", fork->cnid, request.start, request.count);
    
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
    
    char* read_buffer;
    ALLOC(read_buffer, block_count * fork->hfs->block_size);
    ExtentList *extentList = fork->extents;;
    
    // Keep track of what's left to get
    range remaining = request;
    
    while (remaining.count != 0) {
        if (++loopCounter > 2000) {
            Extent *extent = NULL;
            TAILQ_FOREACH(extent, extentList, extents) {
                print("%10zd: %10zd %10zd", extent->logicalStart, extent->startBlock, extent->blockCount);
            }
            PrintExtentList(extentList, fork->totalBlocks);
            critical("We're stuck in a read loop: request (%zd, %zd); remaining (%zd, %zd)", request.start, request.count, remaining.start, remaining.count);
        }
        
//        debug("Remaining: (%zd, %zd)", remaining.start, remaining.count);
        range read_range;
        bool found = extentlist_find(extentList, remaining.start, &read_range.start, &read_range.count);
        if (!found) {
            PrintExtentList(extentList, fork->totalBlocks);
            critical("Logical block %zd not found in the extents for CNID %d!", remaining.start, fork->cnid);
        }
        
        if (read_range.count == 0) {
            warning("About to read a null range! Looking for (%zd, %zd), received (%zd, %zd).", remaining.start, remaining.count, read_range.start, read_range.count);
            continue;
        }
        
        read_range.count = MIN(read_range.count, request.count);
        
        ssize_t blocks = hfs_read_blocks(read_buffer, fork->hfs, read_range.count, read_range.start);
        if (blocks < 0) {
            FREE(read_buffer);
            perror("read fork");
            critical("Read error.");
            return -1;
        }
        
        remaining.count -= MIN(blocks, remaining.count);
        remaining.start += blocks;
            
        if (remaining.count == 0) break;
    }
    
    memcpy(buffer, read_buffer, MIN(block_count, request.count) * fork->hfs->block_size);
    FREE(read_buffer);
    
    return request.count;
}

// Grab a specific byte range of a fork.
ssize_t hfs_read_fork_range(void* buffer, const HFSFork *fork, size_t size, size_t offset)
{
    ASSERT_PTR(buffer);
    ASSERT_PTR(fork);
    
    size_t start_block = 0;
    size_t byte_offset = 0;
    size_t block_count = 0;
    char* read_buffer = NULL;
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
    
    // Use the calculated size instead of the passed size to account for block alignment.
    ALLOC(read_buffer, block_count * fork->hfs->block_size);
    
    // Fetch the data into a read buffer (it may fail).
    read_blocks = hfs_read_fork(read_buffer, fork, block_count, start_block);
    
    // On success, append the data to the buffer (consumers: set buffer.offset properly!).
    if (read_blocks) {
        memcpy(buffer, (read_buffer + byte_offset), size);
    }
    
    // Clean up.
    FREE(read_buffer);
    
    // The amount we added to the buffer.
    return size;
}

#pragma mark funopen - HFSFork

typedef struct HFSForkCookie {
    off_t      cursor;
    HFSFork     *fork;
} HFSForkCookie;

#if defined (BSD)
int fork_readfn(void * c, char * buf, int nbytes)
#else
ssize_t fork_readfn(void * c, char * buf, size_t nbytes)
#endif
{
    HFSForkCookie *cookie = (HFSForkCookie*)c;
    off_t offset = cookie->cursor;
    size_t bytes = 0;
    
    bytes = hfs_read_fork_range(buf, cookie->fork, nbytes, offset);
    if (bytes > 0) cookie->cursor += bytes;
    
    return bytes;
}

#if defined (BSD)
fpos_t fork_seekfn(void * c, fpos_t pos, int mode)
{
#else
int fork_seekfn(void * c, off_t *p, int mode)
{
    off_t pos = *p;
#endif
    HFSForkCookie *cookie = (HFSForkCookie*)c;
    
    switch (mode) {
        case SEEK_CUR:
            pos += cookie->cursor;
            break;
        case SEEK_END:
            pos = (cookie->fork->logicalSize - pos);
            
        default:
            break;
    }
    cookie->cursor = pos;
    
#if defined (BSD)
    return pos;
#else
    *p = pos;
    return 0;
#endif
}

int fork_closefn(void * c)
{
    HFSForkCookie *cookie = (HFSForkCookie*)c;
    free(cookie->fork);
    free(cookie);
    return 0;
}

FILE* fopen_hfsfork(HFSFork* fork)
{
    HFSForkCookie *cookie = calloc(1, sizeof(HFSForkCookie));
    cookie->fork = calloc(1, sizeof(HFSFork));
    *cookie->fork = *fork;
    
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
