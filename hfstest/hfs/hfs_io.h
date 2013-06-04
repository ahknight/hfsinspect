//
//  hfs_io.h
//  hfstest
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfstest_hfs_io_h
#define hfstest_hfs_io_h

#include "hfs_structs.h"

HFSFork make_hfsfork            (const HFSVolume *hfs, const HFSPlusForkData forkData, hfs_fork_type forkType, hfs_node_id cnid);

ssize_t hfs_read_raw            (Buffer *buffer, const HFSVolume *hfs, size_t size, off_t offset);
ssize_t hfs_read_blocks         (Buffer *buffer, const HFSVolume *hfs, size_t block_count, size_t start_block);

ssize_t hfs_read_from_extent    (Buffer *buffer, const HFSVolume *hfs, const HFSPlusExtentDescriptor *extent, size_t block_count, size_t start_block);
ssize_t hfs_read_fork           (Buffer *buffer, const HFSFork *fork, size_t block_count, size_t start_block);
ssize_t hfs_read_fork_range     (Buffer *buffer, const HFSFork *fork, size_t size, off_t offset);


#endif
