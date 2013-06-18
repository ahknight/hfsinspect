//
//  hfs.c
//  hfsinspect
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

    bool success = hfs_get_HFSPlusVolumeHeader(&hfs->vh, hfs);
    if (!success) critical("Could not read volume header!");
    
    if (hfs->vh.signature == kHFSSigWord) {
        error("This tool does not suport HFS Standard volumes.");
        errno = EFTYPE;
        return -1;
    }
    
    if (hfs->vh.signature != kHFSPlusSigWord && hfs->vh.signature != kHFSXSigWord) {
        error("not an HFS+ or HFSX volume signature: 0x%x", hfs->vh.signature);
        VisualizeData((void*)&hfs->vh, sizeof(HFSPlusVolumeHeader));
        errno = EFTYPE;
        return -1;
    }
    
    return 0;
}

int hfs_close(HFSVolume *hfs) {
    debug("Closing volume.");
    int result = close(hfs->fd);
    hfs->fd = 0;
    hfs->fp = NULL;
    return result;
}

#pragma mark Volume Structures

bool hfs_get_HFSPlusVolumeHeader(HFSPlusVolumeHeader* vh, const HFSVolume* hfs)
{
    if (hfs->fd) {
        char* buffer = malloc(4096);
        
        ssize_t size;
//        size = hfs_read_raw(&buffer, hfs, 2048, 0); // Breaks on raw devices.
        size = fread(buffer, 2048, 1, hfs->fp);
        
        if (size < 1) {
            perror("read");
            critical("Cannot read volume.");
            free(buffer);
            return -1;
        }
        
        *vh = *(HFSPlusVolumeHeader*)(buffer+1024);
        free(buffer);
        
        swap_HFSPlusVolumeHeader(vh);
        
        return true;
    }
    
    return false;
}

bool hfs_get_JournalInfoBlock(JournalInfoBlock* block, const HFSVolume* hfs)
{
    if (hfs->vh.journalInfoBlock) {
        void* buffer = malloc(hfs->vh.blockSize);
        ssize_t read = hfs_read_blocks(buffer, hfs, 1, hfs->vh.journalInfoBlock);
        if (read < 0) {
            perror("read");
            critical("Read error when fetching journal info block");
        } else if (read < 1) {
            critical("Didn't read the whole journal info block!");
        }
        *block = *(JournalInfoBlock*)buffer; // copies
        free(buffer);
        
        swap_JournalInfoBlock(block);
        return true;
    }
    
    return false;
}
