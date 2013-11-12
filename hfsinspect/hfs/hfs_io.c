//
//  hfs_io.c
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs_io.h"
#include "hfs_extent_ops.h"
#include "range.h"
#include "output_hfs.h"
#include "hfs_btree.h"

#pragma mark Struct Conveniences

HFSFork hfsfork_get_special(const HFSVolume *hfs, hfs_node_id cnid)
{
    HFSPlusForkData forkData;
    
//	kHFSBadBlockFileID		= 5,	/* File ID of the bad allocation block file */
//	kHFSStartupFileID		= 7,	/* File ID of the startup file (HFS Plus only) */

    switch (cnid) {
        case kHFSExtentsFileID:
            forkData = hfs->vh.extentsFile;
            break;
            
        case kHFSCatalogFileID:
            forkData = hfs->vh.catalogFile;
            break;
            
//        case kHFSExtentsFileID:
//            forkData = hfs->vh.;
//            break;
            
        case kHFSAllocationFileID:
            forkData = hfs->vh.allocationFile;
            break;
            
        case kHFSStartupFileID:
            forkData = hfs->vh.startupFile;
            break;
            
        case kHFSAttributesFileID:
            forkData = hfs->vh.attributesFile;
            break;
            
        default:
            break;
    }
    
    return hfsfork_make(hfs, forkData, HFSDataForkType, cnid);
}

HFSFork hfsfork_make(const HFSVolume *hfs, const HFSPlusForkData forkData, hfs_fork_type forkType, hfs_node_id cnid)
{
    debug("Creating fork for CNID %d and fork %d", cnid, forkType);
    
    HFSFork fork;
    fork.hfs = *hfs;
    fork.forkData = forkData;
    fork.forkType = forkType;
    fork.cnid = cnid;
    fork.totalBlocks = forkData.totalBlocks;
    fork.logicalSize = forkData.logicalSize;
    
    fork.extents = extentlist_alloc();
    bool success = hfs_extents_get_extentlist_for_fork(fork.extents, &fork);
    if (!success) critical("Failed to get extents for new fork!");
    
//    if (getenv("DEBUG")) PrintForkExtentsSummary(&fork);
    
    return fork;
}

void hfsfork_free(HFSFork *fork)
{
    extentlist_free(fork->extents);
}

#pragma mark - Volume I/O

ssize_t hfs_read_raw(void* buffer, const HFSVolume *hfs, size_t size, size_t offset)
{
    offset += hfs->vol->offset;
    info("Reading %zu bytes at offset %zd.", size, offset);
    
    ssize_t result;
    result = fseeko(hfs->vol->fp, offset, SEEK_SET);
    if (result < 0) {
        perror("fseek");
        critical("read error");
    }
    
    result = fread(buffer, sizeof(char), size, hfs->vol->fp);
    if (result < 0) {
        perror("fread");
        critical("read error");
    }
    
    return result;
}

// Block arguments are relative to the volume.
ssize_t hfs_read_blocks(void* buffer, const HFSVolume *hfs, size_t block_count, size_t start_block)
{
    debug("Reading %u blocks starting at block %u", block_count, start_block);
    debug("volume block size: %lu ; disk block size: %lu", hfs->block_size, hfs->vol->block_size);
    unsigned ratio = hfs->block_size / hfs->vol->block_size;
    return vol_read_blocks(hfs->vol, buffer, block_count*ratio, start_block*ratio);
    
    if (hfs->block_count && start_block > hfs->block_count) {
        error("Request for a block past the end of the volume (%d, %d)", start_block, hfs->block_count);
        errno = ESPIPE; // Illegal seek
        return -1;
    }
    
    // Trim to fit.
    if (hfs->block_count && hfs->block_count < (start_block + block_count)) {
        block_count = hfs->block_count - start_block;
    }
    
    size_t  offset  = start_block * hfs->block_size;
    size_t  size    = block_count * hfs->block_size;
    
//    debug("Reading %zd bytes at volume offset %zd.", size, offset);
    ssize_t bytes_read = hfs_read_raw(buffer, hfs, size, offset);
    if (bytes_read == -1) {
        perror("read blocks error");
        critical("read error");
        return bytes_read;
    }
//    debug("read %zd bytes", bytes_read);
    
    // This layer thinks in blocks. Blocks in, blocks out.
    int blocks_read = 0;
    if (bytes_read > 0) {
        blocks_read = MAX(bytes_read / hfs->block_size, 1);
    }
    return blocks_read;
}

ssize_t hfs_read_range(void* buffer, const HFSVolume *hfs, size_t size, size_t offset)
{
    debug("Reading from volume at (%d, %d)", offset, size);
    
    return vol_read(hfs->vol, buffer, size, offset);
    
    // Range check.
    if (hfs->length && offset > hfs->length) {
        error("Request for logical offset larger than the size of the volume (%d, %d)", offset, hfs->length);
        errno = ESPIPE; // Illegal seek
        return -1;
    }
    
    if ( hfs->length && (offset + size) > hfs->length ) {
        size = hfs->length - offset;
        debug("Adjusted read to (%d, %d)", offset, size);
    }
    
    if (size < 1) {
        error("Zero-size request.");
        errno = EINVAL;
        return -1;
    }
    
    // The range starts somewhere in this block.
    size_t start_block = (size_t)(offset / hfs->block_size);
    
    // Offset of the request within the start block.
    size_t byte_offset = (offset % hfs->block_size);
    
    // Add a block to the read if the offset is not block-aligned.
    size_t block_count = (size / hfs->block_size) + ( ((offset + size) % hfs->block_size) ? 1 : 0);
    
    // Use the calculated size instead of the passed size to account for block alignment.
    char* read_buffer; INIT_BUFFER(read_buffer, block_count * hfs->block_size);
    
    // Fetch the data into a read buffer (it may fail).
    ssize_t read_blocks = hfs_read_blocks(read_buffer, hfs, block_count, start_block);
    
    // On success, copy the output.
    if (read_blocks) memcpy(buffer, read_buffer + byte_offset, size);
    
    // Clean up.
    FREE_BUFFER(read_buffer);
    
    // The amount we added to the buffer.
    return size;
}

#pragma mark - Fork I/O

ssize_t hfs_read_fork(void* buffer, const HFSFork *fork, size_t block_count, size_t start_block)
{
    int loopCounter = 0; // Fail-safe.
    
    // Keep the original request around
    range request = make_range(start_block, block_count);
    
    debug("Reading from CNID %u (%d, %d)", fork->cnid, request.start, request.count);
    
    // Sanity checks
    if (request.count < 1) {
        error("Invalid request size: %u blocks", request.count);
        return -1;
    }
    
    if ( request.start > fork->totalBlocks ) {
        error("Request would begin beyond the end of the file (start block: %u; file size: %u blocks.", request.start, fork->totalBlocks);
        return -1;
    }
    
    if ( range_max(request) >= fork->totalBlocks ) {
        request.count = fork->totalBlocks - request.start;
        request.count = MAX(request.count, 1);
        debug("Trimmed request to (%d, %d) (file only has %d blocks)", request.start, request.count, fork->totalBlocks);
    }
    
    char* read_buffer;
    INIT_BUFFER(read_buffer, block_count * fork->hfs.block_size);
    ExtentList *extentList = fork->extents;;
    
    // Keep track of what's left to get
    range remaining = request;
    
    while (remaining.count != 0) {
        if (++loopCounter > 2000) {
            Extent *extent = NULL;
            TAILQ_FOREACH(extent, extentList, extents) {
                printf("%10zd: %10zd %10zd\n", extent->logicalStart, extent->startBlock, extent->blockCount);
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
        
//        debug("Next section: (%zd, %zd)", read_range.start, read_range.count);
        
        ssize_t blocks = hfs_read_blocks(read_buffer, &fork->hfs, read_range.count, read_range.start);
        if (blocks < 0) {
            FREE_BUFFER(read_buffer);
            perror("read fork");
            critical("Read error.");
            return -1;
        }
        
        remaining.count -= MIN(blocks, remaining.count);
        remaining.start += blocks;
            
        if (remaining.count == 0) break;
    }
//    debug("Appending bytes");
    memcpy(buffer, read_buffer, MIN(block_count, request.count) * fork->hfs.block_size);
    FREE_BUFFER(read_buffer);
    
//    debug("returning %zd", request.count);
    return request.count;
}

// Grab a specific byte range of a fork.
ssize_t hfs_read_fork_range(void* buffer, const HFSFork *fork, size_t size, size_t offset)
{
    debug("Reading from fork at (%d, %d)", offset, size);
    
    // Range check.
    if (offset > fork->logicalSize) {
        PrintHFSPlusForkData(&fork->forkData, fork->cnid, fork->forkType);
        error("Request for logical offset larger than the size of the fork (%d, %d)", offset, fork->logicalSize);
        errno = ESPIPE; // Illegal seek
        return -1;
    }
    
    if ( (offset + size) > fork->logicalSize ) {
        size = fork->logicalSize - offset;
        debug("Adjusted read to (%d, %d)", offset, size);
    }
    
    if (size < 1) {
        error("Zero-size request.");
        errno = EINVAL;
        return -1;
    }
    
    // The range starts somewhere in this block.
    size_t start_block = (size_t)(offset / fork->hfs.block_size);
    
    // Offset of the request within the start block.
    size_t byte_offset = (offset % fork->hfs.block_size);
    
    // Add a block to the read if the offset is not block-aligned.
    size_t block_count = (size / fork->hfs.block_size) + ( ((offset + size) % fork->hfs.block_size) ? 1 : 0);
    
    // Use the calculated size instead of the passed size to account for block alignment.
    char* read_buffer; INIT_BUFFER(read_buffer, block_count * fork->hfs.block_size);
    
    // Fetch the data into a read buffer (it may fail).
    ssize_t read_blocks = hfs_read_fork(read_buffer, fork, block_count, start_block);
    
    // On success, append the data to the buffer (consumers: set buffer.offset properly!).
    if (read_blocks) {
        memcpy(buffer, (read_buffer + byte_offset), size);
    }
    
    // Clean up.
    FREE_BUFFER(read_buffer);
    
    // The amount we added to the buffer.
    return size;
}
