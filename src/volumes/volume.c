//
//  volume.c
//  volumes
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "volume.h"
#include "misc/output.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __APPLE__
#include <sys/disk.h>
#endif

#define ASSERT_VOL(vol) { assert(vol); assert(vol->fp); }

Volume* vol_open(const String path, int mode, off_t offset, size_t length, size_t block_size)
{
    int fd;
    char* modestr = NULL;
    struct stat s;
    Volume* vol = NULL;
    
    mode = O_RDONLY;
    modestr = "r";
    
    fd = open(path, mode);
    if (fd < 0)
        return NULL;
    
    if ( fstat(fd, &s) < 0 )
        return NULL;
    
    vol = calloc(1, sizeof(Volume));
    if (vol == NULL)
        return NULL;
    
    memset(vol, 0, sizeof(Volume));
    
    vol->fd = fd;
    vol->fp = fdopen(fd, modestr);
    (void)strlcpy(vol->device, path, PATH_MAX);
    vol->offset = offset;
    
    if (length && block_size) {
        vol->length = length;
        vol->sector_size = block_size;
        vol->sector_count = (length / block_size);
    } else {
        vol->length         = s.st_size;
        vol->sector_size    = s.st_blksize;
        vol->sector_count   = s.st_blocks;
    }
    
#ifdef __APPLE__
    // Try and get the real hardware sector size and use that.
    if (block_size == 0) {
        blkcnt_t  bc = 0;
        blksize_t bs = 0;
        ioctl(vol->fd, DKIOCGETBLOCKCOUNT, &bc);
        ioctl(vol->fd, DKIOCGETBLOCKSIZE, &bs);
        
        vol->sector_count    = ( (bc != 0) ? bc : s.st_blocks);
        vol->sector_size     = ( (bs != 0) ? bs : s.st_blksize);
    }
#endif
    
    if (length == 0 && vol->sector_size && vol->sector_count)
        vol->length = vol->sector_size * vol->sector_count;

    return vol;
}

Volume* vol_qopen(const String path)
{
    Volume *vol = vol_open(path, O_RDONLY, 0, 0, 0);
    return vol;
}

ssize_t vol_read (const Volume *vol, void* buf, size_t size, off_t offset)
{
    ASSERT_VOL(vol);

    debug("Reading from volume at (%jd, %zu)", (intmax_t)offset, size);
    
    // Range checks
    if (vol->length && offset > vol->length)
        return 0;
    
    if ( vol->length && (offset + size) > vol->length ) {
        size = vol->length - offset;
        debug("Adjusted read to (%jd, %zu)", (intmax_t)offset, size);
    }
    
    if (size < 1)
        return 0;
    
    // The range starts somewhere in this block.
    size_t start_block = (size_t)(offset / vol->sector_size);
    
    // Offset of the request within the start block.
    size_t byte_offset = (offset % vol->sector_size);
    
    // Add a block to the read if the offset is not block-aligned.
    size_t block_count = (size / vol->sector_size) + ( ((offset + size) % vol->sector_size) ? 1 : 0);
    
    // Use the calculated size instead of the passed size to account for block alignment.
    Bytes read_buffer; ALLOC(read_buffer, block_count * vol->sector_size);
    
    // Fetch the data into a read buffer (it may fail).
    ssize_t read_blocks = vol_read_blocks(vol, read_buffer, block_count, start_block);
    
    // On success, copy the output.
    if (read_blocks) memcpy(buf, read_buffer + byte_offset, size);
    
    // Clean up.
    FREE(read_buffer);
    
    // The amount we added to the buffer.
    return size;
}

ssize_t vol_read_blocks (const Volume *vol, void* buf, ssize_t block_count, ssize_t start_block)
{
    ASSERT_VOL(vol);
    
//    debug("Reading %u blocks starting at block %u", block_count, start_block);
    if (vol->sector_count && start_block > vol->sector_count)
        return 0;
    
    // Trim to fit.
    if (vol->sector_count && vol->sector_count < (start_block + block_count)) {
        block_count = vol->sector_count - start_block;
    }
    
    if (block_count < 0)
        return 0;
    
    ssize_t bytes_read = 0;
    size_t  offset  = start_block * vol->sector_size;
    size_t  size    = block_count * vol->sector_size;
    
//    debug("Reading %zd bytes at volume offset %zd.", size, offset);
    if ( (bytes_read = vol_read_raw(vol, buf, size, offset)) < 0) {
        perror("vol_read_raw");
        return bytes_read;
    }

//    debug("read %zd bytes", bytes_read);
    
    // Blocks in, blocks out.
    ssize_t blocks_read = 0;
    if (bytes_read > 0) {
        blocks_read = MAX(bytes_read / vol->sector_size, 1);
    }
    return blocks_read;
}

ssize_t vol_read_raw (const Volume *vol, void* buf, size_t nbyte, off_t offset)
{
    ASSERT_VOL(vol);
    
    ssize_t result;
    if ( (result = pread(vol->fd, buf, nbyte, (offset + vol->offset))) < 0)
        perror("pread");
    
    return result;
}

ssize_t vol_write(Volume *vol, const void* buf, size_t nbyte, off_t offset)
{
    ASSERT_VOL(vol);
    
    ssize_t result;
    if ( (result = pwrite(vol->fd, buf, nbyte, (offset + vol->offset))) < 0)
        perror("pwrite");
    
    return result;
}

int vol_close(Volume *vol)
{
    ASSERT_VOL(vol);
    
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
    { if (vol == NULL || vol->fp == NULL) { errno = EINVAL; return NULL; } }
    
    Volume* newvol = calloc(1, sizeof(Volume));
    
    newvol->fd = dup(vol->fd);
    newvol->fp = fdopen(vol->fd, "r"); // Copies are read-only for now.
    memcpy(newvol->device, vol->device, PATH_MAX);
    
    newvol->offset = offset;
    newvol->length = length;
    
    newvol->sector_size = vol->sector_size;
    newvol->sector_count = length / vol->sector_size;
    
    newvol->type = kTypeUnknown;
    newvol->subtype = kTypeUnknown;
    
    // Link the two together
    newvol->parent_partition = vol;
    newvol->depth = vol->depth + 1;
    
    vol->partition_count++;
    vol->partitions[pos] = newvol;
    
    return newvol;
}

void vol_dump(Volume* vol)
{
    if (vol == NULL) {
        error("Volume must not be NULL");
        return;
    }
    
    BeginSection("Volume '%s' (%s)", vol->desc, vol->native_desc);
    
    Print("device", "%s", vol->device);
    PrintUI(vol, type);
    
#define PrintUICharsIfEqual(var, val) if (var == val) { \
    char type[5]; \
    uint64_t i = val; \
    format_uint_chars(type, (const char*)&i, 4, 5); \
    PrintAttribute(#var, "'%s' (%s)", type, #val); \
}

    PrintUICharsIfEqual(vol->type, kTypeUnknown);
    PrintUICharsIfEqual(vol->type, kVolTypeSystem);
    PrintUICharsIfEqual(vol->type, kVolTypePartitionMap);
    PrintUICharsIfEqual(vol->type, kVolTypeUserData);

    PrintUICharsIfEqual(vol->subtype, kTypeUnknown);
    
    PrintUICharsIfEqual(vol->subtype, kPMTypeMBR);
    PrintUICharsIfEqual(vol->subtype, kPMTypeGPT);
    PrintUICharsIfEqual(vol->subtype, kPMTypeAPM);
    PrintUICharsIfEqual(vol->subtype, kPMCoreStorage);

    PrintUICharsIfEqual(vol->subtype, kFSTypeMFS);
    PrintUICharsIfEqual(vol->subtype, kFSTypeHFS);
    PrintUICharsIfEqual(vol->subtype, kFSTypeHFSPlus);
    PrintUICharsIfEqual(vol->subtype, kFSTypeHFSX);
    PrintUICharsIfEqual(vol->subtype, kFSTypeWrappedHFSPlus);

    PrintDataLength(vol, offset);
    PrintDataLength(vol, length);
    
    PrintDataLength(vol, sector_size);
    PrintUI(vol, sector_count);
    
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
    EndSection();
}
