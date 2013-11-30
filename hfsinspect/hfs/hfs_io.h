//
//  hfs_io.h
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_io_h
#define hfsinspect_hfs_io_h

#include "hfs_structs.h"


#pragma mark HFS Volume

ssize_t hfs_read                (void* buffer, const HFS *hfs, size_t size, size_t offset);
ssize_t hfs_read_blocks         (void* buffer, const HFS *hfs, size_t block_count, size_t start_block);

FILE*   fopen_hfs               (HFS* hfs);

#pragma mark HFS Fork

int     hfsfork_get_special     (HFSFork** fork, const HFS *hfs, bt_nodeid_t cnid);

int     hfsfork_make            (HFSFork** fork, const HFS *hfs, const HFSPlusForkData forkData, hfs_forktype_t forkType, bt_nodeid_t cnid);
void    hfsfork_free            (HFSFork* fork);

ssize_t hfs_read_fork           (void* buffer, const HFSFork *fork, size_t block_count, size_t start_block);
ssize_t hfs_read_fork_range     (void* buffer, const HFSFork *fork, size_t size, size_t offset);

FILE*   fopen_hfsfork           (HFSFork* fork);

#endif
