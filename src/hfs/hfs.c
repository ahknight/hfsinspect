//
//  hfs.c
//  hfsinspect
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs/hfs.h"
#include "logging/logging.h" // console printing routines


#pragma mark Volume Abstractions

int hfs_load_mbd(Volume* vol, HFSMasterDirectoryBlock* mdb)
{
    trace("vol (%p), mdb (%p)", vol, mdb);

    // On IA32, using Clang, the swap function needs a little scratch space
    // so we read into a larger area, swap there, then copy out.

    char* buf[200] = {0};

    if ( vol_read(vol, buf, sizeof(HFSMasterDirectoryBlock), 1024) < 0)
        return -1;

    swap_HFSMasterDirectoryBlock((HFSMasterDirectoryBlock*)buf);

    memcpy(mdb, buf, sizeof(HFSMasterDirectoryBlock));

    return 0;
}

int hfsplus_load_header(Volume* vol, HFSPlusVolumeHeader* vh)
{
    trace("vol (%p), vh (%p)", vol, vh);

    if ( vol_read(vol, vh, sizeof(HFSPlusVolumeHeader), 1024) < 0)
        return -1;

    swap_HFSPlusVolumeHeader(vh);

    return 0;
}

int hfs_close(HFSPlus* hfs) {
    trace("hfs (%p)", hfs);
    debug("Closing volume.");
    int result = vol_close(hfs->vol);
    return result;
}

int hfs_open(HFSPlus* hfs, Volume* vol)
{
    int type   = 0;
    int result = 0;

    trace("hfs (%p), vol (%p)", hfs, vol);

    if ((hfs == NULL) || (vol == NULL)) { errno = EINVAL; return -1; }

    // Test to see if we support the volume.
    result = hfs_test(vol);
    if (result < 0) return -1;

    type   = (unsigned)result;

    if ((type != kFSTypeHFSPlus) && (type != kFSTypeWrappedHFSPlus) && (type != kFSTypeHFSX)) {
        errno = EINVAL;
        return -1;
    }

    // Clear the HFSVolume struct (hope you didn't need that)
    memset(hfs, 0, sizeof(struct HFSPlus));

    // Handle wrapped volumes.
    if (type == kFSTypeWrappedHFSPlus) {
        HFSMasterDirectoryBlock mdb = {0};
        if ( hfs_load_mbd(vol, &mdb) < 0)
            return -1;

        hfs->offset = (mdb.drAlBlSt * 512) + (mdb.drEmbedExtent.startBlock * mdb.drAlBlkSiz);
    }

    // Load the volume header.
    if ( hfsplus_load_header(vol, &hfs->vh) < 0 )
        return -1;

    // Update the HFSVolume struct.
    hfs->vol         = vol;
    hfs->block_size  = hfs->vh.blockSize;
    hfs->block_count = hfs->vh.totalBlocks;

    hfs->offset     += vol->offset;
    hfs->length      = (vol->length ? vol->length : hfs->block_size * hfs->block_count);

    return 0;
}

/**
   Tests to see if a volume is HFS or not.
   @return Returns -1 on failure or a VolumeSubtype constant representing the detected filesystem.
 */
int hfs_test(Volume* vol)
{
    int                     type = kTypeUnknown;
    HFSMasterDirectoryBlock mdb  = {0};
    HFSPlusVolumeHeader     vh   = {0};

    trace("vol (%p)", vol);

    debug("hfs_test");

    assert(vol != NULL);

    // First, test for HFS or wrapped HFS+ volumes.
    if ( hfs_load_mbd(vol, &mdb) < 0) {
        return -1;
    }

    if (mdb.drSigWord == kHFSSigWord) {
        if (mdb.drEmbedSigWord == kHFSPlusSigWord) {
            info("Found an HFS-wrapped HFS+ volume");
            type = kFSTypeWrappedHFSPlus;
        } else {
            info("Found an HFS volume.");
            type = kFSTypeHFS;
        }
    }

    if (type != kTypeUnknown)
        return type;

    // Now test for a modern HFS+ volume.
    if ( hfsplus_load_header(vol, &vh) < 0 )
        return -1;

    if ((vh.signature == kHFSPlusSigWord) || (vh.signature == kHFSXSigWord)) {
        info("Found an HFS+ volume");
        return kFSTypeHFSPlus;
    }

    debug("Unknown volume type: %#x", vh.signature);

    return type;
}

/** returns the first HFS+ volume in a tree of volumes */
Volume* hfsplus_find(Volume* vol)
{
    Volume* result = NULL;
    int     test   = 0;

    trace("vol (%p)", vol);

    debug("hfsplus_find");

    assert(vol != NULL);
    test = hfs_test(vol);

    if ( (test) & (kFSTypeHFSPlus | kFSTypeWrappedHFSPlus)) {
        result = vol;
    } else if (vol->partition_count) {
        for(unsigned i = 0; i < vol->partition_count; i++) {
            if (vol->partitions[i] != NULL) {
                result = hfsplus_find(vol->partitions[i]);
                if (result != NULL)
                    break;
            }
        }
    }

    return result;
}

#pragma mark Volume Structures

bool hfs_get_HFSMasterDirectoryBlock(HFSMasterDirectoryBlock* vh, const HFSPlus* hfs)
{
    void*   buffer = NULL;
    ssize_t size   = 0;

    trace("vh (%p), hfs (%p)", vh, hfs);

    if (hfs->vol) {
        SALLOC(buffer, 2048)

        size = hfs_read(buffer, hfs, 2048, 0);

        if (size < 1) {
            perror("read");
            critical("Cannot read volume.");
            SFREE(buffer);
            return -1;
        }

        void* mdbp = (char*)buffer+1024;
        *vh = *(HFSMasterDirectoryBlock*)mdbp;
        SFREE(buffer);

        swap_HFSMasterDirectoryBlock(vh);

        return true;
    }

    return false;
}

bool hfsplus_get_JournalInfoBlock(JournalInfoBlock* block, const HFSPlus* hfs)
{
    trace("block (%p), hfs (%p)", block, hfs);

    if (hfs->vh.journalInfoBlock) {
        void*   buffer     = NULL;
        ssize_t bytes_read = 0;

        SALLOC(buffer, hfs->block_size);

        bytes_read = hfs_read_blocks(buffer, hfs, 1, hfs->vh.journalInfoBlock);
        if (bytes_read < 0) {
            perror("hfs_read_blocks");
            critical("Read error when fetching journal info block");
        } else if (bytes_read < 1) {
            critical("Didn't read the whole journal info block!");
        }
        *block = *(JournalInfoBlock*)buffer; // copies
        SFREE(buffer);

        swap_JournalInfoBlock(block);
        return true;
    }

    return false;
}

bool hfsplus_get_journalheader(journal_header* header, JournalInfoBlock* info, const HFSPlus* hfs)
{
    ssize_t nbytes = 0;

    trace("header (%p), info (%p), hfs (%p)", header, info, hfs);

    if (info->flags & kJIJournalInFSMask && info->offset) {
        nbytes = hfs_read(header, hfs, sizeof(journal_header), info->offset);
        if (nbytes < 0) {
            perror("hfs_read");
            return false;
        }
        return true;
    }

    return false;
}

