//
//  volume.c
//  hfsinspect
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include "volume.h"

Volume* vol_open(char* path, int mode, off_t offset, size_t length, size_t block_size)
{
    Volume* vol = NULL;
    vol = malloc(sizeof(Volume));
    memset(vol, 0, sizeof(Volume));
    
    int result = open(path, mode);
    if (result < 0) {
        free(vol);
        return NULL;
    }
    vol->fd = result;
    return vol;
}

#define VALID_DESCRIPTOR(vol) { if (vol == NULL || vol->fd < 1) { errno = EINVAL; return -1; } }

int vol_read (Volume *vol, void* buf, size_t nbyte, off_t offset)
{
    VALID_DESCRIPTOR(vol);
    
    return pread(vol->fd, buf, nbyte, (offset + vol->offset));
}

int vol_write(Volume *vol, void* buf, size_t nbyte, off_t offset)
{
    VALID_DESCRIPTOR(vol);
    
    return pwrite(vol->fd, buf, nbyte, (offset + vol->offset));
}

int vol_close(Volume *vol)
{
    VALID_DESCRIPTOR(vol);
    
    int result = 0;
    result = close(vol->fd);
    if (result < 0) {
        return -1;
    }
    
    if (malloc_size(vol)) { free(vol); }
    
    return result;
}
