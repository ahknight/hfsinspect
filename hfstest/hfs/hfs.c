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
#include <wchar.h>
#include <xlocale.h>
#include <signal.h>
#include <malloc/malloc.h>

#include "hfs_endian.h"
#include "hfs_unicode.h"
#include "hfs_pstruct.h"
#include "hfs.h"


#pragma Struct Conveniences

HFSFork make_hfsfork(const HFSVolume *hfs, const HFSPlusForkData forkData, u_int8_t forkType, u_int32_t cnid)
{
    HFSFork fork = {};
    fork.hfs = *hfs;
    fork.forkData = forkData;
    fork.forkType = forkType;
    fork.cnid = cnid;
    
    return fork;
}

#pragma mark Volume Abstractions

int hfs_open(HFSVolume *hfs, const char *path) {
	hfs->fd = open(path, O_RDONLY);
    
	if (hfs->fd == -1) return -1;
    
    return hfs->fd;
}

int hfs_load(HFSVolume *hfs) {
    Buffer buffer = buffer_alloc(sizeof(HFSPlusVolumeHeader));
    
    ssize_t size = hfs_read(hfs, &buffer, buffer.size, 1024);
    if (size) {
        hfs->vh = *((HFSPlusVolumeHeader*)(buffer.data));
        swap_HFSPlusVolumeHeader(&hfs->vh);
        
        if (hfs->vh.signature != kHFSPlusSigWord && hfs->vh.signature != kHFSXSigWord) {
            error("error: not an HFS+ or HFX volume signature: 0x%x\n", hfs->vh.signature);
            errno = EFTYPE;
            return -1;
        }
        
        return 0;
    } else {
        return -1;
    }
}

int hfs_close(const HFSVolume *hfs) {
    return close(hfs->fd);
}

#pragma mark FS I/O

ssize_t hfs_read(const HFSVolume *hfs, Buffer *buffer, size_t size, off_t offset)
{
    ssize_t result = buffer_pread(hfs->fd, buffer, size, offset);
    return result;
}

// Block arguments are relative to the volume.
ssize_t hfs_read_blocks(Buffer *buffer, const HFSVolume *hfs, u_int32_t block_count, u_int32_t start_block)
{
    if (start_block > hfs->vh.totalBlocks) {
        errno = ESPIPE; // Illegal seek
        return -1;
    }
    
    // Trim to fit.
    if (hfs->vh.totalBlocks < (start_block + block_count)) {
        block_count = hfs->vh.totalBlocks - start_block;
    }
    
    off_t   offset  = start_block * hfs->vh.blockSize;
    size_t  size    = block_count * hfs->vh.blockSize;
    
    debug("read_blocks: Reading %zd blocks at volume offset %zd.\n", size, offset);
    ssize_t bytes_read = buffer_pread(hfs->fd, buffer, size, offset);
    if (bytes_read == -1)
        return bytes_read;
    return (bytes_read / hfs->vh.blockSize); // This layer thinks in blocks. Blocks in, blocks out.
}

// Block arguments are relative to the extent.
ssize_t hfs_read_from_extent(Buffer *buffer, const HFSVolume *hfs, const HFSPlusExtentDescriptor *extent, u_int32_t block_count, u_int32_t start_block)
{
    // Need to be able to read at least one block.
    // A zero here is a sign of greater problems elsewhere.
    // Trust me.
    if ( (start_block) > extent->blockCount) {
        errno = ESPIPE;
        return -1;
    }
    
    // Create the real read parameters.
    u_int32_t absolute_start = start_block + extent->startBlock;
    u_int32_t absolute_size = MIN(block_count, extent->blockCount - start_block);
    
    ssize_t blocks_read = hfs_read_blocks(buffer, hfs, absolute_size, absolute_start);
    return blocks_read;
}

// Block arguments are relative to the fork.
ssize_t hfs_read_fork(Buffer *buffer, const HFSFork *fork, u_int32_t block_count, u_int32_t start_block)
{
    if (block_count < 1) {
        error("Invalid request size: %d blocks\n", block_count);
        return -1;
    }
    
    if ( (start_block + block_count) > fork->forkData.totalBlocks ) {
        error("Request would extend beyond the file (request size: %d blocks; file size: %d blocks.\n", start_block + block_count, fork->forkData.totalBlocks);
        return -1;
    }
    
    u_int32_t           next_block                  = start_block;
    int64_t             blocks_remaining            = block_count;
    u_int32_t           known_extents_start_block   = 0;
    u_int32_t           known_extents_block_count   = 0;
    u_int32_t           known_extents_searched_blocks      = 0;
    Buffer              read_buffer                 = buffer_alloc(buffer->size);
    
    HFSPlusExtentRecord *extents = NULL;
    HFSPlusExtentRecord forkExtents;
    memcpy(forkExtents, fork->forkData.extents, sizeof(HFSPlusExtentRecord));
    extents = &forkExtents;
    
    for (int i = 0; i < kHFSPlusExtentDensity; i++) known_extents_block_count += (*extents)[i].blockCount;
    
    while (blocks_remaining > 0 && (next_block + blocks_remaining) < fork->forkData.totalBlocks) {
        
        if ( next_block > (known_extents_start_block + known_extents_block_count) ) {
            // Search extents overflow for additional records.
            HFSPlusExtentRecord     record = {};
            u_int32_t               record_start_block = 0;
            int8_t                found = hfs_find_extent_record(&record, &record_start_block, fork, next_block);
            
            if (!found) {
                error("ERR: Extent for block %d not found.\n", known_extents_start_block + known_extents_block_count);
                return -1;
            } else {
                extents = &record;
                known_extents_start_block = record_start_block;
                known_extents_block_count = 0;
                known_extents_searched_blocks = 0;
                for (int i = 0; i < kHFSPlusExtentDensity; i++) known_extents_block_count += (*extents)[i].blockCount;
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
            
            int32_t     start_record        = next_block - known_extents_start_block;
            int32_t     start_extent        = start_record - known_extents_searched_blocks;
            int32_t     start_read          = start_extent;
            ssize_t     result              = 0;
            
            if (start_read < 0) {
                critical("We missed the read in a previous extent.\n");
            }
            
            // Is the read position in the extent?  No need to bother if not.
            if (start_read < extent.blockCount) {
                u_int32_t read_block_count = MIN(extent.blockCount - start_read, (u_int32_t)blocks_remaining);
                debug("reading %d blocks from extent offset %d\n", read_block_count, start_read);
                result = hfs_read_from_extent(&read_buffer, &fork->hfs, &extent, read_block_count, start_read);
                if (result == -1)
                    return result;
                blocks_remaining    -= result;
                next_block          += result;
            }
            
            known_extents_searched_blocks += extent.blockCount;
            debug("read %zd; remaining: %lld; next block: %u; searched: %d\n", result, blocks_remaining, next_block, known_extents_searched_blocks);
        }
    }
    
    if (blocks_remaining > 0) error("%lld blocks not found! (searched %d)\n", blocks_remaining, next_block);
    
    buffer_append(buffer, read_buffer.data, block_count * fork->hfs.vh.blockSize);
    buffer_free(&read_buffer);
    
    return block_count;
}

// Grab a specific byte range of a fork.
ssize_t hfs_read_fork_range(Buffer *buffer, const HFSFork *fork, size_t size, off_t offset)
{
    // We use this a lot.
    u_int32_t block_size = fork->hfs.vh.blockSize;
    
    // Range check.
    if ((offset + size) > fork->forkData.logicalSize) {
        errno = ESPIPE; // Illegal seek
        return -1;
    }
    
    // The range starts somewhere in this block.
    u_int32_t start_block = (u_int32_t)(offset / (off_t)block_size);

    // Offset of the request within the start block.
    u_int32_t byte_offset = (u_int32_t)(offset % (off_t)block_size);
    
    // Add a block to the read if the offset is not block-aligned.
    u_int32_t block_count = (u_int32_t)(size / (size_t)block_size) + ( ((offset + size) % block_size) ? 1 : 0);
    
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

#pragma mark Tree Abstractions

ssize_t hfs_btree_init(HFSBTree *tree, const HFSFork *fork)
{
	ssize_t size1 = 0, size2 = 0;
    
    // First, init the fork record for future calls.
    tree->fork = *fork;
    
    // Now load up the header node.
    Buffer buffer = buffer_alloc(128); // Only need ~46.
    
    size1 = hfs_read_fork_range(&buffer, &tree->fork, sizeof(BTNodeDescriptor), 0);
    
    tree->nodeDescriptor = *( (BTNodeDescriptor*) buffer.data );
    
    if (size1)
        swap_BTNodeDescriptor(&tree->nodeDescriptor);
    
    // If there's a header record, we'll likely want that as well.
    if (tree->nodeDescriptor.numRecords > 0) {
        buffer_zero(&buffer);
        buffer.offset = 0;
        
        size2 = hfs_read_fork_range(&buffer, fork, sizeof(BTHeaderRec), sizeof(BTNodeDescriptor));
        tree->headerRecord = *( (BTHeaderRec*) buffer.data);
        
        if (size2)
            swap_BTHeaderRec(&tree->headerRecord);
        
        // Then 128 bytes of "user" space for record 2.
        // Then the remainder of the node is allocation bitmap data for record 3.
    }
    
    buffer_free(&buffer);

    return (size1 + size2);
}

int8_t hfs_btree_get_node (HFSBTreeNode *out_node, const HFSBTree *tree, u_int32_t nodeNumber)
{
    if(nodeNumber > tree->headerRecord.totalNodes) {
        error("Node beyond file range.\n");
        return -1;
    }
    
    u_int32_t node_size = (tree->headerRecord.nodeSize / tree->fork.hfs.vh.blockSize); // In blocks
    u_int32_t start_block = nodeNumber * node_size;
    
    HFSBTreeNode node;
    node.blockSize  = tree->headerRecord.nodeSize;
    node.nodeOffset = start_block;
    node.nodeNumber = nodeNumber;
    node.bTree      = *tree;
    node.buffer     = buffer_alloc(node.blockSize);
    
    debug("read_node: reading %zd bytes at logical offset %d\n", node.blockSize, node.nodeOffset);
    ssize_t result = 0;
    result = hfs_read_fork(&node.buffer, &tree->fork, node_size, start_block);
    if (result < 0) {
        return result;
    } else {
        int swap_result = swap_BTreeNode(&node);
        if (swap_result == -1) {
            buffer_free(&node.buffer);
            debug("Byte-swap of node failed.\n");
            errno = EINVAL;
            return -1;
        }
    }
    
//    VisualizeData(node.buffer.data, node.buffer.size);
    *out_node = node;
    return 1;
}

void hfs_btree_free_node (HFSBTreeNode *node)
{
    buffer_free(&node->buffer);
}

u_int16_t hfs_btree_get_catalog_record_type (const HFSBTreeNode *node, u_int32_t i)
{
    if(i >= node->nodeDescriptor.numRecords) return 0;
    
    off_t record_offset = hfs_btree_get_record_offset(node, i);
    
    u_int16_t type = *(u_int16_t *)( node->buffer.data + record_offset );
    
    return type;
}

off_t hfs_btree_get_record_offset (const HFSBTreeNode *node, u_int32_t i)
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
    debug("get_record_offset: %d:%d offset: %d; recordOffset: %d\n", node->nodeNumber, i, offset, recordOffset);
    
    return recordOffset;
}

ssize_t hfs_btree_get_record_size (const HFSBTreeNode *node, u_int32_t i)
{
    if(i > node->nodeDescriptor.numRecords) return 0;
    
    u_int16_t offset = hfs_btree_get_record_offset(node, i);
    u_int16_t nextOffset = hfs_btree_get_record_offset(node, i+1);
    ssize_t recordSize = nextOffset - offset;
    
    return recordSize;
}

Buffer hfs_btree_get_record (const HFSBTreeNode *node, u_int32_t i)
{
    if(i > node->nodeDescriptor.numRecords) return buffer_alloc(0);
    
    ssize_t     size    = hfs_btree_get_record_size(node, i);
    u_int16_t   offset  = hfs_btree_get_record_offset(node, i);
    
    Buffer buffer = buffer_slice(&node->buffer, size, offset);
    
    return buffer;
}

ssize_t hfs_btree_decompose_keyed_record (const HFSBTreeNode *node, const Buffer *record, Buffer *key, Buffer* value)
{
    /*
           ____________________
     06 00 00 36 24 da 00 00 00 04 00 00 00
     00 00 FF FF FF FF FF FF ... 00 00 00 ...
     len   key                   value
     
     Get the length first.
     Use that to get the slice for the key.
     The remaining space is the value.
     */
    
    u_int16_t keyLength = 0;
    
    if (node->bTree.headerRecord.attributes & kBTVariableIndexKeysMask) {
        keyLength = *( (u_int16_t*) record->data );
        
        if (keyLength < kHFSPlusCatalogKeyMinimumLength) keyLength = kHFSPlusCatalogKeyMinimumLength;
        if (keyLength > kHFSPlusCatalogKeyMaximumLength) return keyLength; // Things are bad at this point; probably not a keyed record.
        if (keyLength > record->size) {
            return -1;
        }
        
    } else {
        keyLength = node->bTree.headerRecord.maxKeyLength;
    }
    
    Buffer keyBuffer = buffer_slice(record, keyLength + 2, 0); // +2 includes the key length bytes as well.
    *key = keyBuffer;
    
    off_t valueOffset = keyLength + sizeof(u_int16_t);
    if (valueOffset % 2) (valueOffset)++; // Align on a byte boundary.
    size_t value_size = record->size - keyLength - sizeof(keyLength);
    
    Buffer valueBuffer = buffer_slice(record, value_size, valueOffset);
    *value = valueBuffer;
    
    return keyLength;
}

#pragma Tree Files

HFSBTree hfs_get_catalog_btree(const HFSVolume *hfs)
{
    HFSFork fork = make_hfsfork(hfs, hfs->vh.catalogFile, 0x00, kHFSCatalogFileID);
    
    HFSBTree tree;
    hfs_btree_init(&tree, &fork);
    
    return tree;
}

HFSBTree hfs_get_extents_btree(const HFSVolume *hfs)
{
    HFSFork fork = make_hfsfork(hfs, hfs->vh.extentsFile, 0x00, kHFSExtentsFileID);
    
    HFSBTree tree;
    hfs_btree_init(&tree, &fork);
    
    return tree;
}

HFSBTree hfs_get_attrbutes_btree(const HFSVolume *hfs)
{
    HFSFork fork = make_hfsfork(hfs, hfs->vh.attributesFile, 0x00, kHFSAttributesFileID);
    
    HFSBTree tree;
    hfs_btree_init(&tree, &fork);
    
    return tree;
}


#pragma mark Searching

int8_t hfs_find_catalog_record(HFSBTreeNode *node, u_int32_t *recordID, const HFSVolume *hfs, u_int32_t parentFolder, const wchar_t name[256], u_int8_t nameLength)
{
    HFSPlusCatalogKey catalogKey = {};
    catalogKey.parentID = parentFolder;
    catalogKey.nodeName.length = nameLength;
    for (int i = 0; i < nameLength; i++) catalogKey.nodeName.unicode[i] = name[i];
    catalogKey.keyLength = sizeof(catalogKey.parentID) + catalogKey.nodeName.length;
    
    BTreeKey searchKey = *(BTreeKey*)( &catalogKey );
    
    HFSBTree catalogTree = hfs_get_catalog_btree(hfs);
    HFSBTreeNode searchNode = {};
    u_int8_t searchIndex = 0;
    
    int8_t result = hfs_btree_search_tree(&searchNode, &searchIndex, &catalogTree, &searchKey);
    
    if (result) {
        *node = searchNode;
        *recordID = searchIndex;
        return 1;
    }
    
    return 0;
}

int8_t hfs_find_extent_record(HFSPlusExtentRecord *record, u_int32_t *record_start_block, const HFSFork *fork, u_int32_t startBlock)
{
    HFSVolume hfs = fork->hfs;
    u_int32_t fileID = fork->cnid;
    u_int8_t forkType = fork->forkType;
    
    HFSPlusExtentKey extentKey = {};
    extentKey.keyLength = kHFSPlusExtentKeyMaximumLength;
    extentKey.fileID = fileID;
    extentKey.forkType = forkType;
    extentKey.startBlock = startBlock;
    
    BTreeKey searchKey = *(BTreeKey*)( &extentKey );
    
    HFSBTree extentsTree = hfs_get_extents_btree(&hfs);
    HFSBTreeNode node = {};
    u_int8_t index = 0;
    
    u_int8_t result = hfs_btree_search_tree(&node, &index, &extentsTree, &searchKey);
    
    if (node.buffer.size) {
        Buffer recordBuf, key, value;
        recordBuf = hfs_btree_get_record(&node, index);
        result = hfs_btree_decompose_keyed_record(&node, &recordBuf, &key, &value);
        if (result) {
            HFSPlusExtentKey* returnedExtentKey = ( (HFSPlusExtentKey*) key.data);
            HFSPlusExtentRecord* returnedExtentRecord = ( (HFSPlusExtentRecord*) value.data );
            
            if (returnedExtentKey->fileID != fileID || returnedExtentKey->forkType != forkType || returnedExtentKey->startBlock > startBlock) {
                return -1;
            } else {
                memcpy(*record, returnedExtentRecord, sizeof(HFSPlusExtentRecord));
                *record_start_block = returnedExtentKey->startBlock;
            }
        }
        
        buffer_free(&recordBuf);
        buffer_free(&key);
        buffer_free(&value);
    }
    
    return result;
}

u_int8_t hfs_btree_search_tree(HFSBTreeNode *node, u_int8_t *index, const HFSBTree *tree, const void *searchKey)
{
    u_int32_t currentNode = tree->headerRecord.rootNode;
    u_int32_t result;
    
    while (1) {
        if (currentNode > tree->headerRecord.totalNodes) {
            error("search_tree: Tree only has %d nodes. Aborting search.\n", tree->headerRecord.totalNodes);
            raise(SIGTRAP);
        }
        
        HFSBTreeNode searchNode = {};
        result = hfs_btree_get_node(&searchNode, tree, currentNode);
        if (result == -1) {
            error("search tree: failed to get node %d!\n", currentNode);
            return result;
        }
        
        result = hfs_btree_search_node(index, &searchNode, searchKey);
        
        // If this is a leaf node, there is no more searching that can be done.
        int8_t nodeKind = ((BTNodeDescriptor*)searchNode.buffer.data)->kind;
        if (nodeKind == kBTLeafNode) {
            *node = searchNode; //Return a brand new node.
            break;
        }
        
        // If we didn't find it here, follow the pointer record before the insertion index.
        if (result == 0 && *index != 0) {
            (*index)--;
        }
        
        Buffer record, key, value;
        record = hfs_btree_get_record(&searchNode, *index);
        hfs_btree_decompose_keyed_record(&searchNode, &record, &key, &value);
        
        currentNode = *( (u_int32_t*)value.data );
        if (currentNode == 0) {
            error("search_tree: Pointed to the header node.  That doesn't work...\n");
            debug("search_tree: Record size: %zd\n", record.size);
            VisualizeData(record.data, record.size);
            raise(SIGTRAP);
        }
        
        buffer_free(&searchNode.buffer);
        buffer_free(&record);
        buffer_free(&key);
        buffer_free(&value);
    }
    
    // Either index is the node, or the insertion point where it would go.
    return result;
}

// Returns 1/0 for found and the index where the record is/would be.
u_int8_t hfs_btree_search_node(u_int8_t *index, const HFSBTreeNode *node, const void *searchKey)
{
    // Perform a binary search within the records (since they are guaranteed to be ordered).
    int low = 0;
    int high = node->nodeDescriptor.numRecords - 1;
    int result = 0;
    int recNum;
    
    while (low <= high) {
        recNum = (low + high) >> 1;
        off_t offset = hfs_btree_get_record_offset(node, recNum);
        if (offset == -1) raise(SIGTRAP);
        
        if (node->bTree.fork.cnid == kHFSCatalogFileID) {
            HFSPlusCatalogKey *testKey = (HFSPlusCatalogKey*) (node->buffer.data + offset);
            result = hfs_btree_compare_catalog_keys((HFSPlusCatalogKey*)searchKey, testKey);
            
        } else if (node->bTree.fork.cnid == kHFSExtentsFileID) {
            HFSPlusExtentKey *testKey = (HFSPlusExtentKey*) (node->buffer.data + offset);
            result = hfs_btree_compare_extent_keys((HFSPlusExtentKey*)searchKey, testKey);
            
        } else if (node->bTree.fork.cnid == kHFSAttributesFileID) {
            
        } else {
            // It's broke.
            raise(SIGTRAP);
            ;;
        }
        
        if (result < 0) {
            high = recNum - 1;
        } else if (result > 0) {
            low = recNum + 1;
        } else {
            *index = recNum;
            return 1;
        }
    }
    *index = recNum;
    return 0;
}

int hfs_btree_compare_catalog_keys(const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2)
{
    if (key1->parentID == key2->parentID) {
        return FastUnicodeCompare(key1->nodeName.unicode, key1->nodeName.length, key2->nodeName.unicode, key2->nodeName.length);
    } else if (key1->parentID > key2->parentID) return 1;
    
    return -1;
}

int hfs_btree_compare_extent_keys(const HFSPlusExtentKey *key1, const HFSPlusExtentKey *key2)
{
    if (key1->fileID == key2->fileID) {
        if (key1->forkType == key2->forkType) {
            if (key1->startBlock == key2->startBlock) {
                return 0; // equality
            } else if (key1->startBlock > key2->startBlock) return 1;
        } else if (key1->forkType == key2->forkType) return 1;
    } else if (key1->fileID > key2->fileID) return 1;
    
    return -1;
}
