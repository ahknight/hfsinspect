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


int hfs_open(HFSVolume *hfs, const char *path)
{
	hfs->fd = open(path, O_RDONLY);
	
	if (hfs->fd == -1) {
//		printf("Error: %d", errno);
        perror("abort");
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
    if (BE16(hfs->vh.signature) != kHFSPlusSigWord && BE16(hfs->vh.signature) != kHFSXSigWord) {
        printf("abort: not an HFS+ or HFX volume signature: 0x%x\n", hfs->vh.signature);
        exit(-1);
    }
    return 0;
}

int hfs_close(HFSVolume *hfs)
{
    return close(hfs->fd);
}

ssize_t hfs_read(HFSVolume *hfs, void *buf, size_t size, off_t offset)
{
    printf("READ: %lld (%zd)", offset, size);
    return pread(hfs->fd, buf, size, offset);
}

ssize_t hfs_readfork(HFSFork *fork, void *buf, size_t size, off_t offset)
{
    HFSVolume hfs = fork->hfs;
    HFSPlusForkData fd = fork->forkData;
    
    u_int32_t block_start = BYTES_TO_BLOCKS(offset, hfs);
    
    for (int i = 0; i < 8; i++) {
        u_int32_t start = fd.extents[i].startBlock;
        u_int32_t end = start + fd.extents[i].blockCount;
    }
    
    return 0;
}
