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

HFSFork hfsfork_get_special     (const HFSVolume *hfs, hfs_node_id cnid);

HFSFork hfsfork_make            (const HFSVolume *hfs, const HFSPlusForkData forkData, hfs_fork_type forkType, hfs_node_id cnid);
void    hfsfork_free            (HFSFork *fork);

ssize_t hfs_read_raw            (void* buffer, const HFSVolume *hfs, size_t size, size_t offset);
ssize_t hfs_read_blocks         (void* buffer, const HFSVolume *hfs, size_t block_count, size_t start_block);
ssize_t hfs_read_range          (void* buffer, const HFSVolume *hfs, size_t size, size_t offset);

ssize_t hfs_read_fork           (void* buffer, const HFSFork *fork, size_t block_count, size_t start_block);
ssize_t hfs_read_fork_range     (void* buffer, const HFSFork *fork, size_t size, size_t offset);


#endif
