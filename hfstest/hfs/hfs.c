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

#include "hfs.h"


#pragma mark Volume Abstractions

int hfs_open(HFSVolume *hfs, const char *path)
{
	hfs->fd = open(path, O_RDONLY|O_NONBLOCK);
	
	if (hfs->fd == -1) {
		return -1;
	}
	
    return hfs->fd;
}

int hfs_load(HFSVolume *hfs)
{
    ssize_t size = hfs_read(hfs, &hfs->vh, sizeof(hfs->vh), 1024);
    if (size < 0) {
        return -1;
    }

    swap_HFSPlusVolumeHeader(&hfs->vh);

    if (hfs->vh.signature != kHFSPlusSigWord && hfs->vh.signature != kHFSXSigWord) {
        printf("error: not an HFS+ or HFX volume signature: 0x%x\n", hfs->vh.signature);
        errno = EFTYPE;
        return -1;
    }

    return 0;
}

int hfs_close(HFSVolume *hfs)
{
    return close(hfs->fd);
}

#pragma mark Tree Abstractions

ssize_t hfs_btree_init(HFSBTree *tree, HFSFork *fork)
{
    // First, init the fork record for future calls.
    tree->fork = *fork;
    
    // Now load up the header node.
	size_t size = hfs_readfork(fork, &tree->nodeDescriptor, sizeof(BTNodeDescriptor), 0);
    if (size == -1) {
        perror("aborted read of catalog node");
        return errno;
    }
    swap_BTNodeDescriptor(&tree->nodeDescriptor);
    
    // If there's a header record, we'll likely want that as well.
    if (tree->nodeDescriptor.numRecords > 0) {
        size = hfs_readfork(fork, &tree->headerRecord, sizeof(BTHeaderRec), sizeof(BTNodeDescriptor));
        if (size == -1) {
            perror("aborted read of catalog header record");
            exit(errno);
        }
        swap_BTHeaderRec(&tree->headerRecord);
        
        // Then 128 bytes of "user" space for record 2.
        // Then the remainder of the node is allocation bitmap data for record 3.
    }

    return 0;
}

ssize_t hfs_btree_get_node(HFSBTree *tree, HFSBTreeNode *node, u_int32_t nodenum)
{
    off_t offset = tree->headerRecord.nodeSize * nodenum;
    node->tree = *tree;
    node->offset = offset;
    node->nodeNumber = nodenum;
    
    ssize_t result = hfs_readfork(&tree->fork, &node->nodeDescriptor, sizeof(BTNodeDescriptor), offset);
    if (result == -1) {
        perror("read node");
        return errno;
    }
    swap_BTNodeDescriptor(&node->nodeDescriptor);
    
    return 0;
}

off_t hfs_btree_get_record_offset(HFSBTreeNode *node, u_int32_t i)
{
    if(i >= node->nodeDescriptor.numRecords) return -1;
    
    // The offset of the current node in the BTree
    off_t node_offset = node->offset;
    
    // The location of the offset for the requested record in the node
    off_t record_offset_offset = node->tree.headerRecord.nodeSize - (2 * i); // offsets are two bytes
    
    // A place to keep the offset
    u_int16_t record_offset;
    
    // Fetch the offset
    ssize_t read_result = hfs_readfork(&node->tree.fork, &record_offset, sizeof(record_offset), (node_offset + record_offset_offset));
    if (read_result == -1) {
        perror("read node record offset");
        return read_result;
    }
    
    return record_offset;
}

u_int16_t hfs_btree_get_catalog_leaf_record_type(HFSBTreeNode *node, u_int32_t i)
{
    if(i >= node->nodeDescriptor.numRecords) return 0;
    
    u_int16_t type = 0;
    off_t record_offset = hfs_btree_get_record_offset(node, i);
    ssize_t bytes = hfs_readfork(&node->tree.fork, &type, sizeof(type), (node->offset + record_offset));
    if (bytes == -1) {
        perror("read catalog leaf record type field");
        return bytes;
    }
    return type;
}

ssize_t hfs_btree_print_record(HFSBTreeNode* node, u_int32_t i)
{
    if(i >= node->nodeDescriptor.numRecords) return -1;
    
    // The offset of the current node in the BTree
    off_t node_offset = node->offset;
    
    // The offset for the requested record in the node
    off_t record_offset = hfs_btree_get_record_offset(node, i);
    if (record_offset == -1) {
        return record_offset;
    }

    // To get a record we need to know the kind of node so we know what record to expect.
    if (node->tree.fork.cnid == kHFSCatalogFileID) {
        // We're using the catalog file
        if (node->nodeDescriptor.kind == kBTHeaderNode) {
            // Catalog header node. We've got a header record at 0 and nothing useful to us after that.

        } else if (node->nodeDescriptor.kind == kBTIndexNode) {
            // Catalog index node. We've got catalog keys.
            HFSPlusCatalogKey key;
            ssize_t bytes = hfs_readfork(&node->tree.fork, &key, sizeof(key), (node_offset + record_offset));
            if (bytes == -1) {
                perror("read catalog key record");
                return bytes;
            }
            
            // Create a very crude and inaccurate 8-bit representation of the name by dropping the left word of the UTF16BE character.
            char nodename[256];
            for (int j = 0; j<=key.nodeName.length; j++) {
                nodename[j] = key.nodeName.unicode[j+1];
                nodename[j+1] = '\0';
            }
            printf("Catalog key: %d %d %s\n", key.keyLength, key.parentID, nodename);
        } else if (node->nodeDescriptor.kind == kBTLeafNode) {
            // Data node.  Get the type, pick the right struct, then hit it again.
            u_int16_t type = 0;
            ssize_t bytes = hfs_readfork(&node->tree.fork, &type, sizeof(type), (node_offset + record_offset));
            if (bytes == -1) {
                perror("read key type field");
                return bytes;
            }
            
            switch (type) {
                case kHFSPlusFolderRecord:
                {
                    HFSPlusCatalogFolder record;
                    bytes = hfs_readfork(&node->tree.fork, &record, sizeof(record), (node_offset + record_offset));
                    if (bytes == -1) {
                        perror("read key type field");
                        return bytes;
                    }
                    printf("::Folder ID %d has %d items.\n", record.folderID, record.valence);
                }
                    break;
                case kHFSPlusFileRecord:
                {
                    
                }
                    break;
                case kHFSPlusFolderThreadRecord:
                {
                    
                }
                    break;
                case kHFSPlusFileThreadRecord:
                {
                    
                }
                    break;
                    
                default:
                    break;
            }

        } else if (node->nodeDescriptor.kind == kBTMapNode) {
        } else {
            printf("Node type: %d; record %d @ offset %lld\n", node->nodeDescriptor.kind, i, record_offset);
        }
    }
    
    return 0;
}


#pragma mark FS I/O

ssize_t hfs_read(HFSVolume *hfs, void *buf, size_t size, off_t offset)
{
//    printf("READ: %lld (%zd)\n", offset, size);
    return pread(hfs->fd, buf, size, offset);
}

#ifndef	MIN
#define	MIN(a, b)	((a) < (b) ? (a) : (b))
#endif

#ifndef	MAX
#define	MAX(a, b)	((a) > (b) ? (a) : (b))
#endif

ssize_t hfs_read_from_extent(int fd, void* buffer, HFSPlusExtentDescriptor *extent, size_t block_size, size_t size, off_t offset)
{
    off_t range_start_block = offset / block_size;
    if ( range_start_block < extent->blockCount ) {
        off_t extent_size_bytes = extent->blockCount * block_size;
        size_t read_size = MIN(extent_size_bytes, size);
        ssize_t result = pread(fd, buffer, read_size, (extent->startBlock * block_size) + offset);
        if (result == -1) {
            perror("read from extent");
            return -1;
        }
        return result;
    }
    return 0;
}

ssize_t hfs_readfork(HFSFork *fork, void* buffer, size_t size, off_t offset) {
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

//    printf("\n  - Reading %zu bytes starting at %llu\n", size, offset);
    
    ssize_t bytes_read = 0;
    
    for (int i = 0; i < kHFSPlusExtentDensity; i++) {
//        printf("Checking extent %d\n", i);
        
        HFSPlusExtentDescriptor extent = fd->extents[i];
        
        if (size == 0) break;
        if (extent.blockCount == 0) break;
        
        ssize_t result = hfs_read_from_extent(hfs->fd, buffer, &extent, block_size, size, offset);
        if (result == -1) {
            perror("read from fork\n");
            return result;
        }
//        printf("Read %zd bytes\n", result);
        bytes_read += result;
        
        size -= bytes_read;
        if (size == 0) break;
        
        offset += MAX(bytes_read, extent.blockCount * block_size);
//        printf("offset: %lld; size: %zd\n", offset, size);
    }
    
    if (size) printf("ERR: %zd bytes not found! (read %zd)\n", size, bytes_read);
    return bytes_read;
}

//ssize_t hfs_readfork(HFSFork *fork, void *buf, size_t r_size, off_t r_offset)
//{
//    HFSVolume *hfs = &fork->hfs;
//    HFSPlusVolumeHeader *vh = &hfs->vh;
//    HFSPlusForkData *fd = &fork->forkData;
//    
//    if (r_offset < 0) {
//        printf("Invalid start offset: %llu\n", r_offset);
//        return -1;
//    }
//
//    if (r_size < 1) {
//        printf("Invalid read size: %llu\n", r_offset);
//        return -1;
//    }
//
//    printf("Reading %zu bytes starting at %llu\n", r_size, r_offset);
//    
//    /*
//     Iterate over the extents looking for the start block.
//     When found, create a range from that extent with the start offset and size.
//     If this does not satisfy the request size, decrement start and offset by the read size and keep going.
//     When done, iterate over the ranges to copy to the buffer.
//     */
//    
//    struct range {
//        off_t offset;
//        size_t size;
//    };
//    typedef struct range range;
//    
//    // TODO: When the overflow is considered, this needs to be dynamic up to the file's total extents.
//    range ranges[kHFSPlusExtentDensity] = {};
//    
//    // Cache this
//    size_t block_size = vh->blockSize;
//    
//    // Copy the request data so we can mutate this on every iteration.
//    off_t offset = r_offset;
//    size_t size = r_size;
//    
//    size_t bytesRemaining = fork->forkData.logicalSize;
//    size_t blocksRemaining = fork->forkData.totalBlocks;
//    
//    
//    for (int i = 0; i < kHFSPlusExtentDensity; i++) {
//        HFSPlusExtentDescriptor *extent = &fd->extents[i];
//        if (size == 0) break;
//        
//        if (extent->blockCount == 0) {
//            printf("Ran out of extents! (%zu bytes remaining)\n", size);
//            break;
//        }
//        
//        // Get volume offset info for the extents
//        off_t eoffset = BLOCK_TO_OFFSET(extent->startBlock, &hfs);
//        size_t esize = BLOCK_TO_OFFSET(extent->blockCount, &hfs);
//        
//        // Get the logical offset info for the extents
//        off_t loffset = 
//        
//        // Is the current offset in this extent?
//        if (  ) {
//            // Figure out what range of this extent we need for the request, in bytes.
//            
//            // We need to detect and create ranges in the following forms:
//            //          |---x   x------------x   x--|           <-- request (last portion, whole, first portion)
//            // |------------|   |------------|   |------------| <-- extents
//            
//            ranges[i].offset = offset;
//
//            size_t available_bytes = ((extent->startBlock + extent->blockCount) - boffset) * block_size;
//            available_bytes = MIN(available_bytes, (u_int32_t)size);
//            ranges[i].size = available_bytes;
//            printf("Added range (%llu, %zu)\n", ranges[i].offset, ranges[i].size);
//            
//            // Does the data in this extent fulfill the current request?
//            if ( available_bytes == bsize ) break;
//            
//            // Otherwise, update the request and keep going.
//            offset += available_bytes;
//            size -= available_bytes;
//        } else {
//            printf("Block %u not in extent (%u, %u).\n", boffset, extent->startBlock, extent->blockCount);
//        }
////        printf("offset: %llu; size: %zu\n", offset, size);
//    }
//    
//    ssize_t read_size = 0;
//    for (int i = 0; i < kHFSPlusExtentDensity; i++) {
//        if (ranges[i].size == 0) break;
//        
////        printf("Reading %zu bytes starting at %llu\n", ranges[i].size, ranges[i].offset);
//        read_size = hfs_read(hfs, buf, ranges[i].size, ranges[i].offset);
//        
//        if (read_size == -1) return read_size;
//    }
//    
////    printf("Read complete.");
//    return read_size;
//}
