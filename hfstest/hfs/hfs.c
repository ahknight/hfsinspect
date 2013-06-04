//
//  hfs.c
//  hfstest
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs.h"


#pragma mark Volume Abstractions

int hfs_open(HFSVolume *hfs, const char *path) {
    debug("Opening path %s", path);
    hfs->fp = fopen(path, "r");
    if (hfs->fp == NULL)    return -1;
    
	hfs->fd = fileno(hfs->fp); //open(path, O_RDONLY);
	if (hfs->fd == -1)      return -1;
    
    return hfs->fd;
}

int hfs_load(HFSVolume *hfs) {
    debug("Loading volume with descriptor %u", hfs->fd);
    Buffer buffer = buffer_alloc(4096); //sizeof(HFSPlusVolumeHeader));
    
    ssize_t size;
    size = fread(buffer.data, 2048, 1, hfs->fp);

    if (size < 1) {
        perror("read");
        critical("Cannot read volume.");
        return -1;
    }
    HFSPlusVolumeHeader *vh;
    vh = (HFSPlusVolumeHeader*)(buffer.data+1024);
    hfs->vh = *vh;
    
    swap_HFSPlusVolumeHeader(&hfs->vh);
    
    if (hfs->vh.signature != kHFSPlusSigWord && hfs->vh.signature != kHFSXSigWord) {
        error("not an HFS+ or HFX volume signature: 0x%x", hfs->vh.signature);
        VisualizeData((void*)buffer.data, buffer.size);
        errno = EFTYPE;
        return -1;
    }
    
    return 0;
}

int hfs_close(const HFSVolume *hfs) {
    debug("Closing volume.");
    return close(hfs->fd);
}

