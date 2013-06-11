//
//  hfs_io.c
//  hfstest
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs_io.h"
#include "hfs_extent_ops.h"
#include "hfs_pstruct.h"
#include "range.h"

#pragma Struct Conveniences

HFSFork hfsfork_make(const HFSVolume *hfs, const HFSPlusForkData forkData, hfs_fork_type forkType, hfs_node_id cnid)
{
    debug("Creating fork for CNID %d and fork %d", cnid, forkType);
    
    HFSFork fork = {};
    fork.hfs = *hfs;
    fork.forkData = forkData;
    fork.forkType = forkType;
    fork.cnid = cnid;
    
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

#pragma mark FS I/O

ssize_t hfs_read_raw(void* buffer, const HFSVolume *hfs, size_t size, size_t offset)
{
    info("Reading %zu bytes at offset %zd.", size, offset);
    ssize_t result = pread(hfs->fd, buffer, size, offset);
    if (result < 0) {
        perror("read raw");
        critical("read error");
    }
    debug("read %zd bytes", result);
    return result;
}

// Block arguments are relative to the volume.
ssize_t hfs_read_blocks(void* buffer, const HFSVolume *hfs, size_t block_count, size_t start_block)
{
    debug("Reading %u blocks starting at block %u", block_count, start_block);
    if (start_block > hfs->vh.totalBlocks) {
        error("Request for a block past the end of the volume (%d, %d)", start_block, hfs->vh.totalBlocks);
        errno = ESPIPE; // Illegal seek
        return -1;
    }
    
    // Trim to fit.
    if (hfs->vh.totalBlocks < (start_block + block_count)) {
        block_count = hfs->vh.totalBlocks - start_block;
    }
    
    size_t  offset  = start_block * hfs->vh.blockSize;
    size_t  size    = block_count * hfs->vh.blockSize;
    
//    debug("Reading %zd bytes at volume offset %zd.", size, offset);
    ssize_t bytes_read = hfs_read_raw(buffer, hfs, size, offset);
    if (bytes_read == -1) {
        perror("read blocks error");
        critical("read error");
        return bytes_read;
    }
    debug("read %zd bytes", bytes_read);
    return (bytes_read / hfs->vh.blockSize); // This layer thinks in blocks. Blocks in, blocks out.
}

// Block arguments are relative to the extent.
//ssize_t hfs_read_from_extent(Buffer *buffer, const HFSVolume *hfs, const HFSPlusExtentDescriptor *extent, size_t block_count, size_t start_block)
//{
//    debug("Reading %u blocks starting at logical block %u", block_count, start_block);
//    // Need to be able to read at least one block.
//    // A zero here is a sign of greater problems elsewhere.
//    // Trust me.
//    if ( (start_block) > extent->blockCount) {
//        error("Request for block past the end of the extent (%d, %d)", start_block, extent->blockCount);
//        errno = ESPIPE;
//        return -1;
//    }
//    
//    // Create the real read parameters.
//    size_t absolute_start = start_block + extent->startBlock;
//    size_t absolute_size = MIN(block_count, extent->blockCount - start_block);
//    
////    debug("Reading %u blocks starting at allocation block %u", absolute_size, absolute_start);
//    ssize_t blocks_read = hfs_read_blocks(buffer, hfs, absolute_size, absolute_start);
//    return blocks_read;
//}

ssize_t hfs_read_fork_new(void* buffer, const HFSFork *fork, size_t block_count, size_t start_block)
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
    
    if ( request.start > fork->forkData.totalBlocks ) {
        error("Request would begin beyond the end of the file (start block: %u; file size: %u blocks.", request.start, fork->forkData.totalBlocks);
        return -1;
    }
    
    if ( range_max(request) >= fork->forkData.totalBlocks ) {
        request.count = fork->forkData.totalBlocks - request.start;
        request.count = MAX(request.count, 1);
        debug("Trimmed request to (%d, %d) (file only has %d blocks)", request.start, request.count, fork->forkData.totalBlocks);
    }
    
    void* read_buffer  = malloc(block_count * fork->hfs.vh.blockSize);
    ExtentList *extentList = fork->extents;;
    
    // Keep track of what's left to get
    range remaining = request;
    
    while (remaining.count != 0) {
        if (++loopCounter > 2000) {
            Extent *extent = NULL;
            TAILQ_FOREACH(extent, extentList, extents) {
                printf("%10zd: %10zd %10zd\n", extent->logicalStart, extent->startBlock, extent->blockCount);
            }
            PrintExtentList(extentList, fork->forkData.totalBlocks);
            critical("We're stuck in a read loop: request (%zd, %zd); remaining (%zd, %zd)", request.start, request.count, remaining.start, remaining.count);
        }
        
        debug("Remaining: (%zd, %zd)", remaining.start, remaining.count);
        range read_range = {0};
        bool found = extentlist_find(extentList, remaining.start, &read_range.start, &read_range.count);
        if (!found) {
            PrintExtentList(extentList, fork->forkData.totalBlocks);
            critical("Logical block %zd not found in the extents for CNID %d!", remaining.start, fork->cnid);
        }
        
        if (read_range.count == 0) {
            warning("About to read a null range! Looking for (%zd, %zd), received (%zd, %zd).", remaining.start, remaining.count, read_range.start, read_range.count);
            continue;
        }
        
        read_range.count = MIN(read_range.count, request.count);
        
        debug("Next section: (%zd, %zd)", read_range.start, read_range.count);
        
        ssize_t blocks = hfs_read_blocks(read_buffer, &fork->hfs, read_range.count, read_range.start);
        if (blocks < 0) {
            free(read_buffer);
            perror("read fork");
            critical("Read error.");
            return -1;
        }
        
        remaining.count -= MIN(blocks, remaining.count);
        remaining.start += blocks;
            
        if (remaining.count == 0) break;
    }
    debug("Appending bytes");
    memcpy(buffer, read_buffer, MIN(block_count, request.count) * fork->hfs.vh.blockSize);
//    buffer_append(buffer, read_buffer, request.count * fork->hfs.vh.blockSize);
    free(read_buffer);
    
    debug("returning %zd", request.count);
    return request.count;
}

// Block arguments are relative to the fork.
//ssize_t hfs_read_fork(Buffer *buffer, const HFSFork *fork, size_t block_count, size_t start_block)
//{
//    int loopCounter = 0; // Fail-safe.
//    
//    // Keep the original request around
//    range request = make_range(start_block, block_count);
//    
//    debug("Reading from CNID %u (%d, %d)", fork->cnid, request.start, request.count);
//    
//    // Sanity checks
//    if (request.count < 1) {
//        error("Invalid request size: %u blocks", request.count);
//        return -1;
//    }
//    
//    if ( request.start > fork->forkData.totalBlocks ) {
//        error("Request would begin beyond the end of the file (start block: %u; file size: %u blocks.", request.start, fork->forkData.totalBlocks);
//        return -1;
//    }
//    
//    if ( range_max(request) >= fork->forkData.totalBlocks ) {
//        request.count = fork->forkData.totalBlocks - request.start + 1;
//        request.count = MAX(request.count, 1);
//        debug("Trimmed request to (%d, %d) (file only has %d blocks)", request.start, request.count, fork->forkData.totalBlocks);
//    }
//    
//    HFSPlusExtentRecord extents = {};
//    Buffer read_buffer  = buffer_alloc(block_count * fork->hfs.vh.blockSize);
//    
//    // Keep track of what's left to get
//    range remaining = request;
//    
//    // Logical position of extent record in the fork
//    range extent_bounds = {0, 0};
//    
//    while (remaining.count != 0) {
//        loopCounter++; if (loopCounter > 2000) critical("We're stuck in a read loop: request (%zd, %zd); remaining (%zd, %zd)", request.start, request.count, remaining.start, remaining.count);
//        
//        // Load initial extents if we don't have any. Required for lookups in the overflow file.
//        if (extents[0].blockCount == 0) {
//            memcpy(&extents, fork->forkData.extents, sizeof(HFSPlusExtentRecord));
//            extent_bounds.start = 0;
//        }
//        
//        // Process extent record
//        extent_bounds.count = 0;
//        for (int i = 0; i < kHFSPlusExtentDensity; i++) extent_bounds.count += extents[i].blockCount;
//        
//        if ( ! range_contains(remaining.start, extent_bounds)) {
//            // Get another extent.
//            int8_t found = hfs_extents_find_record(&extents, (hfs_block*)&extent_bounds.start, fork, remaining.start);
//            
//            if (found == -1) {
//                critical("Extent for block %d not found.", remaining.start);
//            }
//            
//            for (int i = 0; i < kHFSPlusExtentDensity; i++) {
//                HFSPlusExtentDescriptor descriptor = extents[i];
//                debug("%d Found extent descriptor (%d, %d)", i, descriptor.startBlock, descriptor.blockCount);
//            }
//            
//            continue;
//        }
//        
//        // Start reading through the extent descriptors
//        size_t position = extent_bounds.start;
//        for (int i = 0; i < kHFSPlusExtentDensity; i++) {
//            HFSPlusExtentDescriptor descriptor = extents[i];
//            if (descriptor.blockCount == 0) goto EXIT;
//            range descriptor_bounds = {position, descriptor.blockCount}; // Logical bounds in file
//            ssize_t blocks = 0;
//            
//            // Figure out if this descriptor contains the next block
//            if (range_contains(remaining.start, descriptor_bounds)) {
//                // Where in this descriptor do we start?
//                range read_range = range_intersection(descriptor_bounds, remaining);
//                if (read_range.count == 0) {
//                    critical("About to read a null range!");
//                }
//                debug("Reading range (%d, %d)", read_range.start, read_range.count);
//                
//                blocks = hfs_read_from_extent(&read_buffer, &fork->hfs, &descriptor, read_range.count, read_range.start - descriptor_bounds.start);
//                if (blocks < 0) {
//                    buffer_free(&read_buffer);
//                    perror("read fork");
//                    return -1;
//                }
//            } else {
//                debug("Block %d is not in this descriptor (%d, %d @ %d)", remaining.start, descriptor.startBlock, descriptor.blockCount, descriptor_bounds.start);
//            }
//            
//            position += descriptor_bounds.count; // Position within the record
//            remaining.count -= MIN(blocks, remaining.count);
//            remaining.start += blocks;
//            
//            if (remaining.count == 0) break;
//        }
//    }
//    
//EXIT:
//    buffer_append(buffer, read_buffer.data, request.count * fork->hfs.vh.blockSize);
//    buffer_free(&read_buffer);
//    
//    return request.count;
//}

// Grab a specific byte range of a fork.
ssize_t hfs_read_fork_range(Buffer *buffer, const HFSFork *fork, size_t size, size_t offset)
{
    debug("Reading from fork at (%d, %d)", offset, size);
    
    // We use this a lot.
    size_t block_size = fork->hfs.vh.blockSize;
    
    // Range check.
    if (offset > fork->forkData.logicalSize) {
        error("Request for logical offset larger than the size of the fork (%d, %d)", offset, fork->forkData.logicalSize);
        errno = ESPIPE; // Illegal seek
        return -1;
    }
    
    if ( (offset + size) > fork->forkData.logicalSize ) {
        size = fork->forkData.logicalSize - offset;
        debug("Adjusted read to (%d, %d)", offset, size);
    }
    
    if (size < 1) {
        error("Zero-size request.");
        errno = EINVAL;
        return -1;
    }
    
    // The range starts somewhere in this block.
    size_t start_block = (size_t)(offset / (size_t)block_size);
    
    // Offset of the request within the start block.
    size_t byte_offset = (offset % block_size);
    
    // Add a block to the read if the offset is not block-aligned.
    size_t block_count = (size / (size_t)block_size) + ( ((offset + size) % block_size) ? 1 : 0);
    
    // Use the calculated size instead of the passed size to account for block alignment.
    void* read_buffer = malloc(block_count * block_size);
    
    // Fetch the data into a read buffer (it may fail).
    ssize_t read_blocks = hfs_read_fork_new(read_buffer, fork, block_count, start_block);
    
    // On success, append the data to the buffer (consumers: set buffer.offset properly!).
    if (read_blocks) buffer_append(buffer, read_buffer + byte_offset, size);
    
    // Clean up.
    free(read_buffer);
    
    // The amount we added to the buffer.
    return size;
}
