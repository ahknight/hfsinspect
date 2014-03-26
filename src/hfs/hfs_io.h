//
//  hfs_io.h
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_io_h
#define hfsinspect_hfs_io_h

#include "hfs/hfs_types.h"

extern hfs_forktype_t HFSDataForkType;
extern hfs_forktype_t HFSResourceForkType;

#pragma mark HFS Volume

ssize_t hfs_read                (void* buffer, const HFS *hfs, size_t size, size_t offset)              _NONNULL;
ssize_t hfs_read_blocks         (void* buffer, const HFS *hfs, size_t block_count, size_t start_block)  _NONNULL;

FILE*   fopen_hfs               (HFS* hfs) _NONNULL;

#pragma mark HFS Fork

int     hfsfork_get_special     (HFSFork** fork, const HFS *hfs, bt_nodeid_t cnid) _NONNULL;

int     hfsfork_make            (HFSFork** fork, const HFS *hfs, const HFSPlusForkData forkData, hfs_forktype_t forkType, bt_nodeid_t cnid) _NONNULL;
void    hfsfork_free            (HFSFork* fork) _NONNULL;

ssize_t hfs_read_fork           (void* buffer, const HFSFork *fork, size_t block_count, size_t start_block)     _NONNULL;
ssize_t hfs_read_fork_range     (void* buffer, const HFSFork *fork, size_t size, size_t offset)                 _NONNULL;

FILE*   fopen_hfsfork           (HFSFork* fork) _NONNULL;

#endif
