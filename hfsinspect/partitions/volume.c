//
//  volume.c
//  hfsinspect
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "volume.h"
#include "output.h"

#include <fcntl.h>
#include <sys/disk.h>
#include <sys/stat.h>
#include <unistd.h>

#define VALID_DESCRIPTOR(vol) { if (vol == NULL || vol->fd < 1) { errno = EINVAL; return -1; } }

Volume* vol_open(const char* path, int mode, off_t offset, size_t length, size_t block_size)
{
    int fd;
    long bc = 0, bs = 0;
    char* modestr = NULL;
    struct stat s;
    Volume* vol = NULL;
    
    mode = O_RDONLY;
    modestr = "r";
    
    fd = open(path, mode);
    if (fd < 0)
        return NULL;
    
    if ( fstat(fd, &s) < 0 )
        return -1;
    
    vol = malloc(sizeof(Volume));
    memset(vol, 0, sizeof(Volume));
    
    vol->fd = fd;
    vol->fp = fdopen(fd, modestr);
    strlcpy(vol->device, path, PATH_MAX);
    vol->offset = offset;
    vol->length = length;
    
    if (block_size == 0) {
        vol->block_size     = ( (ioctl(vol->fd, DKIOCGETBLOCKSIZE, &bs) == 0) ? bs : s.st_blksize);
        vol->block_count    = ( (ioctl(vol->fd, DKIOCGETBLOCKCOUNT, &bc) == 0) ? bc : s.st_blocks);

    } else {
        vol->block_size = block_size;
        vol->block_count = length / block_size;
    }
    
    if (length == 0 && vol->block_size && vol->block_count)
        vol->length = vol->block_size * vol->block_count;

    return vol;
}

Volume* vol_qopen(const char* path)
{
    Volume *vol = vol_open(path, O_RDONLY, 0, 0, 0);
    if (vol == NULL) perror("vol_open");
    
    return vol;
}

ssize_t vol_read (const Volume *vol, void* buf, size_t size, size_t offset)
{
    VALID_DESCRIPTOR(vol);
    
    debug("Reading from volume at (%d, %d)", offset, size);
    
    // Range check.
    if (vol->length && offset > vol->length) {
        error("Request for logical offset larger than the size of the volume (%d, %d)", offset, vol->length);
        errno = ESPIPE; // Illegal seek
        return -1;
    }
    
    if ( vol->length && (offset + size) > vol->length ) {
        size = vol->length - offset;
        debug("Adjusted read to (%d, %d)", offset, size);
    }
    
    if (size < 1) {
        error("Zero-size request.");
        errno = EINVAL;
        return -1;
    }
    
    // The range starts somewhere in this block.
    size_t start_block = (size_t)(offset / vol->block_size);
    
    // Offset of the request within the start block.
    size_t byte_offset = (offset % vol->block_size);
    
    // Add a block to the read if the offset is not block-aligned.
    size_t block_count = (size / vol->block_size) + ( ((offset + size) % vol->block_size) ? 1 : 0);
    
    // Use the calculated size instead of the passed size to account for block alignment.
    char* read_buffer; INIT_BUFFER(read_buffer, block_count * vol->block_size);
    
    // Fetch the data into a read buffer (it may fail).
    ssize_t read_blocks = vol_read_blocks(vol, read_buffer, block_count, start_block);
    
    // On success, copy the output.
    if (read_blocks) memcpy(buf, read_buffer + byte_offset, size);
    
    // Clean up.
    FREE_BUFFER(read_buffer);
    
    // The amount we added to the buffer.
    return size;
}

ssize_t vol_read_blocks (const Volume *vol, void* buf, size_t block_count, size_t start_block)
{
    VALID_DESCRIPTOR(vol);
    
//    debug("Reading %u blocks starting at block %u", block_count, start_block);
    if (vol->block_count && start_block > vol->block_count) {
        error("Request for a block past the end of the volume (%d, %d)", start_block, vol->block_count);
        errno = ESPIPE; // Illegal seek
        return -1;
    }
    
    // Trim to fit.
    if (vol->block_count && vol->block_count < (start_block + block_count)) {
        block_count = vol->block_count - start_block;
    }
    
    ssize_t bytes_read = 0;
    size_t  offset  = start_block * vol->block_size;
    size_t  size    = block_count * vol->block_size;
    
//    debug("Reading %zd bytes at volume offset %zd.", size, offset);
    
    if ( (bytes_read = vol_read_raw(vol, buf, size, offset)) < 0) {
        perror("vol_read_raw");
        return bytes_read;
    }

//    debug("read %zd bytes", bytes_read);
    
    // This function measures in blocks. Blocks in, blocks out.
    ssize_t blocks_read = 0;
    if (bytes_read > 0) {
        blocks_read = MAX(bytes_read / vol->block_size, 1);
    }
    return blocks_read;
}

ssize_t vol_read_raw (const Volume *vol, void* buf, size_t nbyte, off_t offset)
{
    VALID_DESCRIPTOR(vol);
    
    ssize_t result;
    if ( (result = pread(vol->fd, buf, nbyte, (offset + vol->offset))) < 0)
        perror("pread");
    
    return result;
}

ssize_t vol_write(Volume *vol, const void* buf, size_t nbyte, off_t offset)
{
    VALID_DESCRIPTOR(vol);
    
    ssize_t result;
    if ( (result = pwrite(vol->fd, buf, nbyte, (offset + vol->offset))) < 0)
        perror("pwrite");
    
    return result;
}

int vol_close(Volume *vol)
{
    VALID_DESCRIPTOR(vol);
    
    int fd = 0, result = 0;
    unsigned idx = 0;
    
    if (vol->partition_count) {
        while (vol->partition_count) {
            idx = (vol->partition_count - 1);
            vol_close( (vol->partitions[idx]) );
            vol->partitions[idx] = NULL;
            vol->partition_count--;
        }
    }
    
    fd = vol->fd;
    if (malloc_size(vol)) { free(vol); }
    
    if ( (result = close(fd)) < 0)
        perror("close");
    
    return result;
}

Volume* vol_make_partition(Volume* vol, uint16_t pos, off_t offset, size_t length)
{
    VALID_DESCRIPTOR(vol);
    
    Volume* newvol = malloc(sizeof(Volume));
    
    newvol->fd = dup(vol->fd);
    newvol->fp = fdopen(vol->fd, "r"); // Copies are read-only for now.
    memcpy(newvol->device, vol->device, PATH_MAX);
    
    newvol->offset = offset;
    newvol->length = length;
    
    newvol->block_size = vol->block_size;
    newvol->block_count = length / vol->block_size;
    
    newvol->type = kVolumeTypeUnknown;
    newvol->subtype = kVolumeSubtypeUnknown;
    
    // Link the two together
    newvol->parent_partition = vol;
    vol->partition_count++;
    vol->partitions[pos] = newvol;
    
    return newvol;
}

void vol_dump(Volume* vol)
{
    /*
     Device: /dev/disk9
     Offset: 0
     Length: 1000000
     Block Size: 512
     Block Count: 1010101
     Type: kWhatever
     Subtype: kWhatever
     Partition Count: 2
     Partition 1:
        Device: /dev/disk9
        Offset: 100
        ...
     Partition 2:
         Device: /dev/disk9
         Offset: 1000
         ...
     */
    
    // Indent if we're a sub-partition.
    char indent[20] = {'\0'};
    if (vol->parent_partition != NULL) {
        Volume *v = vol->parent_partition;
        while (v != NULL) {
            strlcat(indent, "  ", 20);
            v = v->parent_partition;
        }
    }
    Print("device", "%s", vol->device);
    PrintUI(vol, type);
    PrintConstIfEqual(vol->type, kVolumeTypeUnknown);
    PrintConstIfEqual(vol->type, kVolumeTypePartitionMap);
    PrintConstIfEqual(vol->type, kVolumeTypeFilesystem);
    PrintDataLength(vol, offset);
    PrintDataLength(vol, length);
    PrintDataLength(vol, block_size);
    PrintUI(vol, block_count);
    PrintUI(vol, partition_count);
    if (vol->partition_count) {
        FOR_UNTIL(i, vol->partition_count) {
            if (vol->partitions[i] != NULL) {
                BeginSection("Partition %u:", i+1);
                vol_dump(vol->partitions[i]);
                EndSection();
            }
        }
    }
}
