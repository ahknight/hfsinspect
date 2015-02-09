//
//  volume.c
//  volumes
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <unistd.h>
#include <errno.h>              // errno/perror
#include <fcntl.h>
#include <sys/stat.h>
#include <libgen.h>
#include <assert.h>             // assert()

#include <string.h>             // memcpy, strXXX, etc.
#if defined(__linux__)
    #include <bsd/string.h>     // strlcpy, etc.
#endif

#if defined (__APPLE__)
    #include <sys/disk.h>
#elif defined (__linux__)
    #include <sys/ioctl.h>
    #include <sys/mount.h>
#endif

#include "volume.h"
#include "output.h"
#include "logging/logging.h"    // console printing routines

#define ASSERT_VOL(vol) { assert(vol != NULL); assert(vol->fp); }

int vol_open(Volume* vol, const char* path, int mode, off_t offset, size_t length, size_t block_size)
{
    assert(vol);
    assert(path);

    struct stat s = {0};
    FILE*       f = NULL;

    if ( (f = fopen(path, "rb")) == NULL) {
        return -errno;
    }

    vol->fp = f;
    vol->fd = fileno(f);
    
    if ( fstat(vol->fd, &s) < 0 )
        return -errno;

    vol->mode = s.st_mode;
    
    (void)strlcpy(vol->source, path, PATH_MAX);
    vol->offset = offset;

    if (length && block_size) {
        vol->length       = length;
        vol->sector_size  = block_size;
        vol->sector_count = (length / block_size);
    } else {
        vol->length       = s.st_size;
        vol->sector_size  = S_BLKSIZE;
        vol->sector_count = s.st_blocks;
    }

    if ((block_size == 0) && (S_ISBLK(vol->mode) || S_ISCHR(vol->mode))) {
        // Try and get the real hardware sector size and use that.
        // Note that this helps with devices, but not with disk images.

#if defined (__APPLE__)
        blkcnt_t  bc = 0;
        blksize_t bs = 0;
        blksize_t ps = 0;
        
        ioctl(vol->fd, DKIOCGETBLOCKCOUNT, &bc);
        ioctl(vol->fd, DKIOCGETBLOCKSIZE, &bs);
        ioctl(vol->fd, DKIOCGETPHYSICALBLOCKSIZE, &ps);

        vol->sector_count = ( (bc != 0) ? bc : s.st_blocks);
        vol->sector_size  = ( (bs != 0) ? bs : S_BLKSIZE);
        vol->phy_sector_size = ( (ps != 0) ? ps : vol->sector_size);

#elif defined (__linux__)
        unsigned int  bs = 0;
        unsigned long ds = 0;

        ioctl(vol->fd, BLKSSZGET, &bs);     // BLKBSZGET is the physical sector size; BLKSSZGET is the logical. Probably.
        ioctl(vol->fd, BLKGETSIZE64, &ds);  // Logical device size in bytes

        vol->sector_size  = ( (bs != 0) ? bs : S_BLKSIZE);
        vol->length       = ( (ds != 0) ? ds : s.st_size);
        vol->sector_count = vol->length / vol->sector_size;
#endif
    }

    if ((length == 0) && vol->sector_size && vol->sector_count)
        vol->length = vol->sector_size * vol->sector_count;

    char* name = basename((char*)path);
    strlcpy((char*)&vol->desc, name, 99);
    
    if (S_ISBLK(vol->mode)) {
        strlcpy((char*)vol->native_desc, "block device", 99);
        
    } else if (S_ISCHR(vol->mode)) {
        strlcpy((char*)vol->native_desc, "character device", 99);
        
    } else {
        strlcpy((char*)vol->native_desc, "regular file", 99);
    }
    
    // Setup the output context
    out_ctx* ctx = NULL;
    ALLOC(ctx, sizeof(out_ctx));
    *ctx     = OCMake(0, 2, "volume");
    vol->ctx = ctx;

    return 0;
}

Volume* vol_qopen(const char* path)
{
    Volume* vol = NULL;
    ALLOC(vol, sizeof(Volume));

    if ( vol_open(vol, path, O_RDONLY, 0, 0, 0) < 0 ) {
        FREE(vol);
        // perror("vol_open");
        return NULL;
    }

    return vol;
}

int vol_blk_get(const Volume* vol, off_t start, size_t count, void* buf)
{
    int      rval;
    unsigned blksz = vol->sector_size;
    off_t    off   = start*blksz + vol->offset;

    debug("Seeking to %llu then reading %zu blocks of size %u.", (unsigned long long)off, count, blksz);
    if ( (rval = fseeko(vol->fp, off, SEEK_SET)) < 0 )
        return rval;
    return fread(buf, blksz, count, vol->fp);
}

ssize_t vol_read (const Volume* vol, void* buf, size_t size, off_t offset)
{
    size_t  start_block = 0, byte_offset = 0, block_count = 0;
    ssize_t read_blocks = 0;
    Bytes   read_buffer = NULL;

    ASSERT_VOL(vol);

    debug("Reading from volume %s+%ju at (%jd, %zu)", basename((char*)&vol->source), (uintmax_t)vol->offset, (intmax_t)offset, size);

    // Range checks
    if (vol->length && (offset > vol->length)) {
        debug("Read ignored; beyond end of source.");
        return 0;
    }

    if ( vol->length && ((offset + size) > vol->length)) {
        size = vol->length - offset;
        debug("Adjusted read to (%jd, %zu)", (intmax_t)offset, size);
    }

    if (size < 1) {
        debug("Read ignored; zero length.");
        return 0;
    }

    // Add a block to the read if the offset is not block-aligned.
    block_count = (size / vol->sector_size) + ( ((offset + size) % vol->sector_size) ? 1 : 0);

    // Use the calculated size instead of the passed size to account for block alignment.
    ALLOC(read_buffer, block_count * vol->sector_size);

    // The range starts somewhere in this block.
    start_block = (size_t)(offset / vol->sector_size);

    // Offset of the request within the start block.
    byte_offset = (offset % vol->sector_size);

    // Fetch the data into a read buffer (it may fail).
    // read_blocks = vol_read_blocks(vol, read_buffer, block_count, start_block);
    read_blocks = vol_blk_get(vol, start_block, block_count, read_buffer);

    // Adjust for truncated reads.
    size        = MIN( (read_blocks * vol->sector_size) - byte_offset, size );

    // On success, copy the output.
    if (read_blocks) memcpy(buf, read_buffer + byte_offset, size);

    // Clean up.
    FREE(read_buffer);

    // The amount we added to the buffer.
    return size;
}

int vol_close(Volume* vol)
{
    ASSERT_VOL(vol);

    int      fd  = 0, result = 0;
    unsigned idx = 0;

    if (vol->partition_count) {
        while (vol->partition_count) {
            idx                  = (vol->partition_count - 1);
            vol_close( (vol->partitions[idx]) );
            vol->partitions[idx] = NULL;
            vol->partition_count--;
        }
    }

    fd = vol->fd;
    FREE(vol);

    if ( (result = close(fd)) < 0)
        perror("close");

    return result;
}

Volume* vol_make_partition(Volume* vol, uint16_t pos, off_t offset, size_t length)
{
    { if ((vol == NULL) || (vol->fp == NULL)) { errno = EINVAL; return NULL; } }

    Volume* newvol = NULL;
    ALLOC(newvol, sizeof(Volume));

    if( (newvol->fd = dup(vol->fd)) < 0) {
        FREE(newvol);
        perror("dup");
        return NULL;
    }
    if( (newvol->fp = fdopen(vol->fd, "r")) == NULL) {
        FREE(newvol);
        perror("fdopen");
        return NULL;
    }
    memcpy(newvol->source, vol->source, PATH_MAX);

    newvol->offset           = offset;
    newvol->length           = length;

    newvol->sector_size      = vol->sector_size;
    newvol->sector_count     = length / vol->sector_size;
    newvol->phy_sector_size  = vol->phy_sector_size;

    newvol->type             = kTypeUnknown;
    newvol->subtype          = kTypeUnknown;

    // Link the two together
    newvol->parent_partition = vol;
    newvol->depth            = vol->depth + 1;
    newvol->ctx              = vol->ctx;

    vol->partition_count++;
    vol->partitions[pos]     = newvol;

    return newvol;
}

void vol_dump(Volume* vol)
{
    if (vol == NULL) {
        error("Volume must not be NULL");
        return;
    }

    BeginSection(vol->ctx, "Volume '%s' (%s)", vol->desc, vol->native_desc);

    Print(vol->ctx, "source", "%s", vol->source);

#define PrintUICharsIfEqual(var, val) if (var == val) { \
        char     type[5]; \
        uint64_t i = val; \
        format_uint_chars(type, (const char*)&i, 4, 5); \
        PrintAttribute(vol->ctx, #var, "'%s' (%s)", type, #val); \
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

    PrintDataLength(vol->ctx, vol, offset);
    PrintDataLength(vol->ctx, vol, length);

    PrintUI(vol->ctx, vol, sector_count);
    PrintDataLength(vol->ctx, vol, sector_size);
    PrintDataLength(vol->ctx, vol, phy_sector_size);

    PrintUI(vol->ctx, vol, partition_count);
    if (vol->partition_count) {
        FOR_UNTIL(i, vol->partition_count) {
            if (vol->partitions[i] != NULL) {
                BeginSection(vol->ctx, "Partition %u:", i+1);
                vol_dump(vol->partitions[i]);
                EndSection(vol->ctx);
            } else {
                warning("unexpected NULL partition record in volume structure");
            }
        }
    }
    EndSection(vol->ctx);
}

