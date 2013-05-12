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

#define MAC_GMT_FACTOR      2082844800UL

#define BLOCK_TO_OFFSET(blocks, hfs) (off_t)(blocks * hfs->vh.blockSize)
#define OFFSET_TO_BLOCK(offset, hfs) (u_int32_t)(offset / hfs->vh.blockSize)

int hfs_open(HFSVolume *hfs, const char *path);
int hfs_load(HFSVolume *hfs);
int hfs_close(HFSVolume *hfs);
ssize_t hfs_read(HFSVolume *hfs, void *buf, size_t size, off_t offset);     // Read from disk.
ssize_t hfs_readfork(HFSFork *fork, void *buf, size_t size, off_t offset);  // Read from a fork, following known extents.

ssize_t hfs_btree_init(HFSBTree *tree, HFSFork *fork);
ssize_t hfs_btree_get_node(HFSBTree *tree, HFSBTreeNode *node, u_int32_t nodenum);
ssize_t hfs_btree_print_record(HFSBTreeNode* node, u_int32_t i);

#endif
