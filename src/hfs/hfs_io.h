//
//  hfs_io.h
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_io_h
#define hfsinspect_hfs_io_h

#include "hfs/types.h"

extern hfs_forktype_t HFSDataForkType;
extern hfs_forktype_t HFSResourceForkType;

#pragma mark HFS Volume

ssize_t hfs_read            (void* buffer, const HFSPlus* hfs, size_t size, size_t offset) __attribute__((nonnull));
ssize_t hfs_read_blocks     (void* buffer, const HFSPlus* hfs, size_t block_count, size_t start_block) __attribute__((nonnull));

FILE* fopen_hfs           (HFSPlus* hfs) __attribute__((nonnull));

#pragma mark HFS Fork

int hfsplus_get_special_fork (HFSPlusFork** fork, const HFSPlus* hfs, bt_nodeid_t cnid) __attribute__((nonnull));

int  hfsfork_make        (HFSPlusFork** fork, const HFSPlus* hfs, const HFSPlusForkData forkData, hfs_forktype_t forkType, bt_nodeid_t cnid) __attribute__((nonnull));
void hfsfork_free        (HFSPlusFork* fork) __attribute__((nonnull));

ssize_t hfs_read_fork       (void* buffer, const HFSPlusFork* fork, size_t block_count, size_t start_block) __attribute__((nonnull));
ssize_t hfs_read_fork_range (void* buffer, const HFSPlusFork* fork, size_t size, size_t offset) __attribute__((nonnull));

FILE* fopen_hfsfork       (HFSPlusFork* fork) __attribute__((nonnull));

#endif
