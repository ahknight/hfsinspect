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


#pragma Struct Conveniences

HFSFork make_hfsfork(const HFSVolume *hfs, const HFSPlusForkData forkData, hfs_fork_type forkType, hfs_node_id cnid)
{
    HFSFork fork = {};
    fork.hfs = *hfs;
    fork.forkData = forkData;
    fork.forkType = forkType;
    fork.cnid = cnid;
    
    return fork;
}


#pragma mark FS I/O

ssize_t hfs_read_raw(Buffer *buffer, const HFSVolume *hfs, size_t size, off_t offset)
{
    info("Reading %zu bytes at offset %zd.", size, offset);
    ssize_t result = buffer_pread(hfs->fd, buffer, size, offset);
//    info("pread: %zd bytes.", result);
    return result;
}

// Block arguments are relative to the volume.
ssize_t hfs_read_blocks(Buffer *buffer, const HFSVolume *hfs, size_t block_count, size_t start_block)
{
//    debug("Reading %u blocks starting at block %u", block_count, start_block);
    if (start_block > hfs->vh.totalBlocks) {
        error("Request for a block past the end of the volume (%d, %d)", start_block, hfs->vh.totalBlocks);
        errno = ESPIPE; // Illegal seek
        return -1;
    }
    
    // Trim to fit.
    if (hfs->vh.totalBlocks < (start_block + block_count)) {
        block_count = hfs->vh.totalBlocks - start_block;
    }
    
    off_t   offset  = start_block * hfs->vh.blockSize;
    size_t  size    = block_count * hfs->vh.blockSize;
    
//    debug("Reading %zd bytes at volume offset %zd.", size, offset);
    ssize_t bytes_read = hfs_read_raw(buffer, hfs, size, offset);
    if (bytes_read == -1)
        return bytes_read;
    return (bytes_read / hfs->vh.blockSize); // This layer thinks in blocks. Blocks in, blocks out.
}

// Block arguments are relative to the extent.
ssize_t hfs_read_from_extent(Buffer *buffer, const HFSVolume *hfs, const HFSPlusExtentDescriptor *extent, size_t block_count, size_t start_block)
{
//    debug("Reading %u blocks starting at logical block %u", block_count, start_block);
    // Need to be able to read at least one block.
    // A zero here is a sign of greater problems elsewhere.
    // Trust me.
    if ( (start_block) > extent->blockCount) {
        error("Request for block past the end of the extent (%d, %d)", start_block, extent->blockCount);
        errno = ESPIPE;
        return -1;
    }
    
    // Create the real read parameters.
    size_t absolute_start = start_block + extent->startBlock;
    size_t absolute_size = MIN(block_count, extent->blockCount - start_block);
    
//    debug("Reading %u blocks starting at allocation block %u", absolute_size, absolute_start);
    ssize_t blocks_read = hfs_read_blocks(buffer, hfs, absolute_size, absolute_start);
    return blocks_read;
}

// Block arguments are relative to the fork.
ssize_t hfs_read_fork(Buffer *buffer, const HFSFork *fork, size_t block_count, size_t start_block)
{
//    debug("Reading %u blocks from CNID %u starting at block %u", block_count, fork->cnid, start_block);
//    bool debug_on = (getenv("DEBUG") != 0);
    
    if (block_count < 1) {
        error("Invalid request size: %u blocks", block_count);
        return -1;
    }
    
    if ( start_block > fork->forkData.totalBlocks ) {
        error("Request would begin beyond the end of the file (start block: %u; file size: %u blocks.", start_block, fork->forkData.totalBlocks);
        return -1;
    }
    
    if ( (start_block + block_count) > fork->forkData.totalBlocks ) {
        block_count = fork->forkData.totalBlocks - start_block;
        block_count = MAX(block_count, 1);
        debug("Trimmed request to (%d, %d)", start_block, block_count);
    }
    
//    if (debug_on) {
//        debug("Extents for CNID %u", fork->cnid);
//        PrintExtentsSummary(fork);
//    }
    
    size_t  next_block                     = start_block;
    int64_t blocks_remaining               = block_count;
    size_t  known_extents_start_block      = 0;
    size_t  known_extents_block_count      = 0;
    size_t  known_extents_searched_blocks  = 0;
    Buffer  read_buffer                    = buffer_alloc(buffer->size);
    
    HFSPlusExtentRecord *extents = NULL;
    HFSPlusExtentRecord forkExtents;
    memcpy(forkExtents, fork->forkData.extents, sizeof(HFSPlusExtentRecord));
    extents = &forkExtents;
    
    for (int i = 0; i < kHFSPlusExtentDensity; i++) known_extents_block_count += (*extents)[i].blockCount;
//    debug("Starting read using default extents.");
    
    while (blocks_remaining > 0 && (next_block + blocks_remaining) <= fork->forkData.totalBlocks) {
//        debug("Entering read loop with %u blocks left out of %u.", blocks_remaining, fork->forkData.totalBlocks);
        if ( next_block > (known_extents_start_block + known_extents_block_count) ) {
//            debug("Looking for a new extent record in the overflow file.");
            // Search extents overflow for additional records.
            HFSPlusExtentRecord record  = {};
            size_t record_start_block   = 0;
            int8_t found                = hfs_extents_find_record(&record, &record_start_block, fork, next_block);
            
            if (found == -1) {
                critical("Extent for block %u not found.", known_extents_start_block + known_extents_block_count);
                return -1;
            } else {
//                debug("Found a new extent record that starts at block %u", record_start_block);
                extents = &record;
                known_extents_start_block = record_start_block;
                known_extents_block_count = 0;
                known_extents_searched_blocks = 0;
                for (int i = 0; i < kHFSPlusExtentDensity; i++) known_extents_block_count += (*extents)[i].blockCount;
                
//                if (debug_on) {
//                    Buffer extentsBuffer = buffer_alloc(sizeof(HFSPlusExtentRecord));
//                    buffer_set(&extentsBuffer, (char*)extents, sizeof(HFSPlusExtentRecord));
//                    PrintHFSPlusExtentRecordBuffer(&extentsBuffer, 1, fork->forkData.totalBlocks);
//                    buffer_free(&extentsBuffer);
//                }
            }
        }
        
        // Read any data from that record.
        for (int i = 0; i < kHFSPlusExtentDensity; i++) {
            /*
             start     position                 count
             30        10                         +40
             0........9
             0........9
             0........9
             0........9
             x----------------------x
             44                    67
             
             next_block is the start of the next read in file blocks.
             read_start is the start of the next read, considering processed extents and next_block.
             read_start above should be 4 as 30 blocks have been processed in a previous record and 10 in this record.
             known_extents_start_block says how many have been processed in previous records.
             known_extents_searched_blocks says how many have been processed in this record.
             */
            
            if (blocks_remaining == 0) break;
            
            HFSPlusExtentDescriptor extent = {};
            extent = (*extents)[i];
            if (extent.blockCount == 0) break;
            
//            debug("Reading extent descriptor %d (%u, %u)", i, extent.startBlock, extent.blockCount);
            
            ssize_t     start_record        = next_block - known_extents_start_block;
            ssize_t     start_extent        = start_record - known_extents_searched_blocks;
            ssize_t     start_read          = start_extent;
            ssize_t     result              = 0;
            
            if (start_read < 0) {
                critical("We missed the read in a previous extent.");
            }
            
            // Is the read position in the extent?  No need to bother if not.
            if (start_read < extent.blockCount) {
                size_t read_block_count = MIN(extent.blockCount - start_read, (size_t)blocks_remaining);
                debug("reading %u blocks from extent offset %d", read_block_count, start_read);
                result = hfs_read_from_extent(&read_buffer, &fork->hfs, &extent, read_block_count, start_read);
                if (result == -1)
                    return result;
                blocks_remaining    -= result;
                next_block          += result;
            } else {
                debug("Skipping read. %d is not in (%d, %d)", start_read, extent.startBlock, extent.blockCount);
            }
            
            known_extents_searched_blocks += extent.blockCount;
//            debug("read %zd; remaining: %lld; next block: %u; searched: %u", result, blocks_remaining, next_block, known_extents_searched_blocks);
        }
    }
    
    if (blocks_remaining > 0) error("%lld blocks not found! (searched %u)", blocks_remaining, next_block);
    
    buffer_append(buffer, read_buffer.data, block_count * fork->hfs.vh.blockSize);
    buffer_free(&read_buffer);
    
    return block_count;
}

// Grab a specific byte range of a fork.
ssize_t hfs_read_fork_range(Buffer *buffer, const HFSFork *fork, size_t size, off_t offset)
{
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
    
    // The range starts somewhere in this block.
    size_t start_block = (size_t)(offset / (off_t)block_size);
    
    // Offset of the request within the start block.
    off_t byte_offset = (offset % block_size);
    
    // Add a block to the read if the offset is not block-aligned.
    size_t block_count = (size / (size_t)block_size) + ( ((offset + size) % block_size) ? 1 : 0);
    
    // Use the calculated size instead of the passed size to account for block alignment.
    Buffer read_buffer = buffer_alloc(block_count * block_size);
    
    // Fetch the data into a read buffer (it may fail).
    ssize_t read_blocks = hfs_read_fork(&read_buffer, fork, block_count, start_block);
    
    // On success, append the data to the buffer (consumers: set buffer.offset properly!).
    if (read_blocks) buffer_append(buffer, read_buffer.data + byte_offset, size);
    
    // Clean up.
    buffer_free(&read_buffer);
    
    // The amount we added to the buffer.
    return size;
}
