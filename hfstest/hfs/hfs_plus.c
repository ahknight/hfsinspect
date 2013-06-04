//
//  hfs_plus.c
//  hfstest
//
//  Created by Adam Knight on 5/29/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs_plus.h"
//#include "hfs_structs.h"
#include "hfs_endian.h"
#include "buffer.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>

hfs_fork_type HFSDataForkType = 0x00;
hfs_fork_type HFSResourceForkType = 0xFF;

#pragma mark UDIO Structures

typedef struct hfs_plus_volume_repr {
    FILE                *fp;
    HFSPlusVolumeHeader vh;
} hfs_plus_volume_repr;

#pragma mark Range Helpers

range make_range(off_t start, size_t count)
{
    range r = { .start = start, .count = count };
    return r;
}

bool range_equal(range a, range b)
{
    if (a.start == b.start) {
        if (a.count == b.count) {
            return true;
        }
    }
    return false;
}

bool range_contains(off_t pos, range r)
{
    return (pos > r.start && pos < range_max(r));
}

size_t range_max(range r)
{
    return (r.start + r.count);
}

range range_intersection(range a, range b)
{
    range r = {};
    
    if ( (range_contains(a.start, b) || range_contains(range_max(a), b)) ) {
        r.start = MAX(a.start, b.start);
        r.count = MIN(range_max(a), range_max(b));
    }
    
    return r;
}

#pragma mark Private Interface

// Unless otherwise specified, sizes and offsets are in blocks.
udio_t  hfs_volume_open         (hfs_plus_volume_opts *opts);
ssize_t hfs_volume_read         (udio_t file, char *buf, size_t nbyte);
ssize_t hfs_volume_write        (udio_t file, char *buf, size_t nbyte);
off_t   hfs_volume_seek         (udio_t file, off_t offset, int whence);
int     hfs_volume_close        (udio_t file);

ssize_t hfs_volume_pread        (udio_t file, char *buf, size_t nbyte, off_t offset); //bytes
ssize_t hfs_volume_read_range   (udio_t file, char *buf, range block_range);

ssize_t hfs_volume_header       (HFSPlusVolumeHeader *vh, udio_t file);


struct hfs_plus_volume_ops {
    udio_open_func  hfs_volume_open;
    udio_read_func  hfs_volume_read;
    udio_write_func hfs_volume_write;
    udio_seek_func  hfs_volume_seek;
    udio_close_func hfs_volume_close;
};


#pragma mark Implementation

range _blocks_to_bytes(range blocks, u_int32_t block_size)
{
    range r = {};
    r.start = blocks.start * block_size;
    r.count = blocks.count * block_size;
    return r;
}

range _bytes_to_blocks(range bytes, u_int32_t block_size)
{
    // Find the blocks that contain the start and end bytes.
    // For the end byte, note that any remainder on the start block
    // will be pushed into that block, so bump it by one if that's
    // the case.
    // Get the count by diffing the start/end blocks.
    
    off_t start         = bytes.start;
    size_t count        = bytes.count;
    off_t end           = start + count;
    off_t block_start   = (start / block_size);
    off_t block_end     = (end / block_size) + ( end % block_size ? 1 : 0);
    off_t block_count   = block_end - block_start;
    
    range r = {};
    r.start = block_start;
    r.count = block_count;
    
    return r;
}


#pragma mark Volume Abstractions

/*
 typedef udio_t  (*udio_open_func)     (void* source);
 typedef int     (*udio_read_func)     (udio_t file, char *buf, int nbyte);
 typedef int     (*udio_write_func)    (udio_t file, const char *buf, int nbyte);
 typedef off_t   (*udio_seek_func)     (udio_t file, off_t offset, int whence);
 typedef int     (*udio_close_func)    (udio_t file);
 */

udio_t hfs_volume_open(hfs_plus_volume_opts *opts)
{
    FILE *f = fopen(opts->path, "r");
    if (f == NULL) {
        error("Couldn't open the file %s", opts->path);
        return NULL;
    }
    
    hfs_plus_volume_repr *v = malloc(sizeof(hfs_plus_volume_repr));
    v->fp = f;
    
    ssize_t result = hfs_volume_header(&v->vh, v);
    if (result < 1) {
        error("Error reading volume header.");
        fclose(f);
        free(v);
        return NULL;
    }
    
    swap_HFSPlusVolumeHeader(&v->vh);
    
    if (v->vh.signature != kHFSPlusSigWord && v->vh.signature != kHFSXSigWord) {
        error("Not an HFS+ or HFX volume signature: 0x%x", v->vh.signature);
        errno = EFTYPE;
        return NULL;
    }
    
    return v;
}

ssize_t hfs_volume_read(udio_t file, char *buf, size_t blocks)
{
    debug("Reading %zu blocks.", blocks);
    hfs_plus_volume_repr *v = (hfs_plus_volume_repr*)file;
    size_t nbyte = blocks * v->vh.blockSize;
    
    debug("Reading %zu bytes.", nbyte);
    return (int)read(fileno(v->fp), buf, nbyte);
}

ssize_t hfs_volume_write(udio_t file, char *buf, size_t nbyte)
{
    // Unsupported.
    errno = ENODEV;
    return -1;
}


off_t hfs_volume_seek(udio_t file, off_t block, int whence)
{
    debug("Seeking to block %llu.", block);
    hfs_plus_volume_repr *v = (hfs_plus_volume_repr*)file;
    off_t offset = (block * (off_t)v->vh.blockSize);
    debug("Seeking to byte %llu.", offset);
    return (int)fseeko(v->fp, offset, whence);
}

int hfs_volume_close(udio_t file)
{
    debug("Closing volume.");
    hfs_plus_volume_repr *v = (hfs_plus_volume_repr*)file;
    return fclose(v->fp);
}



// Sizes are in bytes.
ssize_t hfs_volume_pread(udio_t file, char *buf, size_t nbyte, off_t offset)
{
    debug("reading %zu bytes at offset %llu.", nbyte, offset);
    hfs_plus_volume_repr *v = (hfs_plus_volume_repr*)file;
    return pread(fileno(v->fp), buf, nbyte, offset);
}

// Read a range of blocks.
ssize_t hfs_volume_read_range(udio_t file, char *buf, range block_range)
{
    debug("reading %zu blocks at block offset %llu.", block_range.count, block_range.start);
    hfs_plus_volume_repr *v = (hfs_plus_volume_repr*)file;
    range byte_range = _blocks_to_bytes(block_range, v->vh.blockSize);
    debug("reading %zu bytes at offset %llu.", byte_range.count, byte_range.start);
    return hfs_volume_pread(file, buf, byte_range.count, byte_range.start);
}


ssize_t hfs_volume_header(HFSPlusVolumeHeader *vh, udio_t file)
{
    debug("Reading volume header.");
    return hfs_volume_pread(file, (char*)vh, sizeof(HFSPlusVolumeHeader), 1024);
}


#pragma mark Fork Abstractions

struct hfs_fork_repr {
    udio_t              hfs;        // Volume to read from.
    HFSPlusForkData     fork_data;  // Contains size info and first extents.
    u_int32_t           cnid;       // Catalog node ID.
    u_int8_t            fork_type;  // Fork (HFSDataForkType or 0xFF)
    off_t               pos;        // Read position (we don't cascade seeks down).
};
typedef struct hfs_fork_repr hfs_fork_repr;

udio_t  hfs_fork_open   (hfs_fork_opts *opts);
ssize_t hfs_fork_read   (udio_t file, char *buf, size_t nbyte);
ssize_t hfs_fork_write  (udio_t file, char *buf, size_t nbyte);
off_t   hfs_fork_seek   (udio_t file, off_t offset, int whence);
int     hfs_fork_close  (udio_t file);

struct hfs_plus_fork_ops {
    udio_open_func  hfs_fork_open;
    udio_read_func  hfs_fork_read;
    udio_write_func hfs_fork_write;
    udio_seek_func  hfs_fork_seek;
    udio_close_func hfs_fork_close;
};

udio_t hfs_fork_open(hfs_fork_opts *opts)
{
    if (opts->hfs == NULL) {
        errno = EINVAL;
        error("Invalid HFS Plus volume reference.");
        return NULL;
    }

    if (opts->cnid == 0) {
        errno = EINVAL;
        error("Invalid CNID.");
        return NULL;
    }

    if (opts->fork_type != HFSDataForkType && opts->fork_type != 0xFF) {
        errno = EINVAL;
        error("Invalid fork ID.");
        return NULL;
    }

    hfs_fork_repr *f = malloc(sizeof(hfs_fork_repr));
    f->hfs          = opts->hfs;
    f->fork_data    = opts->fork_data;
    f->cnid         = opts->cnid;
    f->fork_type    = opts->fork_type;
    
    return f;
}

off_t hfs_fork_seek(udio_t file, off_t offset, int whence)
{
    debug("Seeking to offset %llu.", offset);
    hfs_fork_repr *f = (hfs_fork_repr*)file;
    // FIXME: Range checks. Set errno, reutrn -1;
    switch (whence) {
        case SEEK_SET:
            f->pos = offset;
            break;
        case SEEK_CUR:
            f->pos += offset;
            break;
        case SEEK_END:
            f->pos = f->fork_data.logicalSize - offset;
            break;
            
        default:
            break;
    }
    return f->pos;
}

