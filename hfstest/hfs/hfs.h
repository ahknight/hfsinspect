//
//  hfs.h
//  hfstest
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfstest_hfs_h
#define hfstest_hfs_h

#include <hfs/hfs_format.h>
#include "hfs_structs.h"
#include "hfs_endian.h"

#define BLOCKS_TO_BYTES(blocks, hfs) (size_t)(blocks * hfs.vh.blockSize)
#define BYTES_TO_BLOCKS(bytes, hfs) (u_int32_t)(bytes / hfs.vh.blockSize)

int hfs_open(HFSVolume *hfs, const char *path);
int hfs_load(HFSVolume *hfs);
int hfs_close(HFSVolume *hfs);
ssize_t hfs_read(HFSVolume *hfs, void *buf, size_t size, off_t offset);     // Read from disk.
ssize_t hfs_readfork(HFSFork *fork, void *buf, size_t size, off_t offset);  // Read from a fork, following known extents.

#endif
