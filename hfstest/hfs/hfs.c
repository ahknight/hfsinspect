//
//  hfs.c
//  hfstest
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <string.h>
#include <assert.h>
#include <malloc/malloc.h>
#include "hfs_endian.h"

#include "hfs.h"


#ifndef	MIN
#define	MIN(a, b)	((a) < (b) ? (a) : (b))
#endif

#ifndef	MAX
#define	MAX(a, b)	((a) > (b) ? (a) : (b))
#endif


#pragma mark Volume Abstractions

int hfs_open(HFSVolume *hfs, const char *path) {
	hfs->fd = open(path, O_RDONLY|O_NONBLOCK);
    
	if (hfs->fd == -1) return -1;
    
    return hfs->fd;
}

int hfs_load(HFSVolume *hfs) {
    Buffer buffer = buffer_alloc(sizeof(HFSPlusVolumeHeader));
    
    ssize_t size = hfs_read(hfs, &buffer, buffer.size, 1024);
    if (size == -1) return -1;
    
    hfs->vh = *((HFSPlusVolumeHeader*)(buffer.data));
    swap_HFSPlusVolumeHeader(&hfs->vh);

    if (hfs->vh.signature != kHFSPlusSigWord && hfs->vh.signature != kHFSXSigWord) {
        printf("error: not an HFS+ or HFX volume signature: 0x%x\n", hfs->vh.signature);
        errno = EFTYPE;
        return -1;
    }

    return 0;
}

int hfs_close(HFSVolume *hfs) {
    return close(hfs->fd);
}

#pragma mark FS I/O

ssize_t hfs_read(HFSVolume *hfs, Buffer *buffer, size_t size, off_t offset)
{
    ssize_t result = buffer_pread(hfs->fd, buffer, size, offset);
    return result;
}

ssize_t hfs_read_from_extent(HFSVolume *hfs, Buffer *buffer, HFSPlusExtentDescriptor *extent, size_t block_size, size_t size, off_t offset)
{
    off_t range_start_block = offset / block_size;
    if ( range_start_block < extent->blockCount ) {
        off_t extent_size_bytes = extent->blockCount * block_size;
        size_t read_size = MIN(extent_size_bytes, size);
        off_t eoffset = (extent->startBlock * block_size) + offset;
        printf("read_extent: reading %zd bytes at physical offset %lld\n", read_size, eoffset);
        
        ssize_t result = hfs_read(hfs, buffer, read_size, eoffset);
        return result;
    }
    return 0;
}

ssize_t hfs_readfork(HFSFork *fork, Buffer *buffer, size_t size, off_t offset) {
    HFSVolume *hfs = &fork->hfs;
    size_t block_size = hfs->vh.blockSize;
    HFSPlusForkData *fd = &fork->forkData;
    
    if (offset < 0) {
        printf("Invalid start offset: %llu\n", offset);
        return -1;
    }
    
    if (size < 1) {
        printf("Invalid read size: %llu\n", offset);
        return -1;
    }
    
    ssize_t bytes_read = 0;
    
    for (int i = 0; i < kHFSPlusExtentDensity; i++) {
        HFSPlusExtentDescriptor extent = fd->extents[i];
        
        if (size == 0) break;
        if (extent.blockCount == 0) break;
        
        printf("read_fork: reading %zd bytes from logical offset %lld\n", size, offset);
        ssize_t result = hfs_read_from_extent(hfs, buffer, &extent, block_size, size, offset);
        if (result == -1) return result;

        bytes_read += result;
        
        size -= bytes_read;
        if (size == 0) break;
        
        offset += MAX(bytes_read, extent.blockCount * block_size);
    }
    
    if (size) printf("ERR: %zd bytes not found! (read %zd)\n", size, bytes_read);
    return bytes_read;
}


#pragma mark Tree Abstractions

ssize_t hfs_btree_init(HFSBTree *tree, HFSFork *fork)
{
	ssize_t size1 = 0, size2 = 0;
    
    // First, init the fork record for future calls.
    tree->fork = *fork;
    
    // Now load up the header node.
    Buffer buffer = buffer_alloc(4096);
    
    size1 = hfs_readfork(&tree->fork, &buffer, sizeof(BTNodeDescriptor), 0);
    
    tree->nodeDescriptor = *( (BTNodeDescriptor*) buffer.data );
    
    buffer_free(&buffer);
    
    if (size1)
        swap_BTNodeDescriptor(&tree->nodeDescriptor);
    
    // If there's a header record, we'll likely want that as well.
    if (tree->nodeDescriptor.numRecords > 0) {
        Buffer buffer = buffer_alloc(4096);
        
        size2 = hfs_readfork(fork, &buffer, sizeof(BTHeaderRec), sizeof(BTNodeDescriptor));
        tree->headerRecord = *( (BTHeaderRec*) buffer.data);
        
        if (size2)
            swap_BTHeaderRec(&tree->headerRecord);
        
        // Then 128 bytes of "user" space for record 2.
        // Then the remainder of the node is allocation bitmap data for record 3.
        
        buffer_free(&buffer);
    }

    return (size1 + size2);
}

BTreeNode hfs_btree_get_node(HFSBTree *tree, u_int32_t nodeNumber)
{
    if(nodeNumber > tree->headerRecord.totalNodes) {
        BTreeNode nope;
        nope.nodeNumber = -1;
        return nope;
    }
    
    off_t offset = tree->headerRecord.nodeSize * nodeNumber;
    
    BTreeNode node;
    node.blockSize  = tree->headerRecord.nodeSize;
    node.nodeOffset = offset;
    node.nodeNumber = nodeNumber;
    node.bTree      = *tree;
    node.buffer     = buffer_alloc(node.blockSize);
    
    printf("read_node: reading %zd bytes at logical offset %lld\n", node.blockSize, node.nodeOffset);
    ssize_t result = hfs_readfork(&tree->fork, &node.buffer, node.blockSize, offset);
    
    if (result)
        swap_BTreeNode(&node);
    
    return node;
}

u_int16_t hfs_btree_get_catalog_record_type(BTreeNode *node, u_int32_t i)
{
    if(i >= node->nodeDescriptor.numRecords) return 0;
    
    off_t record_offset = hfs_btree_get_record_offset(node, i);
    
    u_int16_t type = *(u_int16_t *)( node->buffer.data + record_offset );
    
    return type;
}

u_int16_t hfs_btree_get_record_offset(BTreeNode *node, u_int32_t i)
{
    if(i >= node->nodeDescriptor.numRecords) return -1;
    
    // Deep breath.
    // Take the pointer to the start of the buffer and add the size of the block
    // to go to the end of the buffer and then back up 16 bits for each
    // offset from the first until we're at the one we want and THEN cast that
    // to a pointer and dereference it so we have a u_int16_t.
    // I told you C was fun.
    
    u_int32_t offset = (u_int32_t)node->blockSize - (sizeof(u_int16_t) * (i + 1));
    u_int16_t recordOffset = *(u_int16_t*)( node->buffer.data + offset );
//    printf("offset: %d; recordOffset: %d\n", offset, recordOffset);
    
    return recordOffset;
}

ssize_t hfs_btree_get_record_size   (BTreeNode *node, u_int32_t i)
{
    if(i > node->nodeDescriptor.numRecords) return 0;
    
    u_int16_t offset = hfs_btree_get_record_offset(node, i);
    u_int16_t nextOffset = hfs_btree_get_record_offset(node, i+1);
    ssize_t recordSize = nextOffset - offset;
    
    return recordSize;
}

Buffer hfs_btree_get_record        (BTreeNode *node, u_int32_t i)
{
    if(i > node->nodeDescriptor.numRecords) return buffer_alloc(0);
    
    ssize_t     size    = hfs_btree_get_record_size(node, i);
    u_int16_t   offset  = hfs_btree_get_record_offset(node, i);
    
    Buffer buffer = buffer_slice(&node->buffer, size, offset);
    
    return buffer;
}

ssize_t hfs_btree_decompose_record  (const Buffer *record, Buffer *key, Buffer* value)
{
    Buffer keyBuffer = buffer_slice(record, sizeof(BTreeKey), 0);
    *key = keyBuffer;
    
    BTreeKey btkey = *( (BTreeKey*) keyBuffer.data );
    
    u_int16_t key_length = btkey.length16;

    if (key_length < kHFSCatalogKeyMinimumLength) key_length = kHFSCatalogKeyMinimumLength; // FIXME: Might break other btrees.
    if (key_length % 2) (key_length)++;

    size_t value_size = record->size - key_length;
    
    Buffer valueBuffer = buffer_slice(record, value_size, key_length);
    *value = valueBuffer;
    buffer_resize(key, key_length);
    buffer_resize(value, value_size);
    
    return key_length;
}

