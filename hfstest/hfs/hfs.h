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
#include "buffer.h"

#define MAC_GMT_FACTOR      2082844800UL

int         hfs_open                            (HFSVolume *hfs, const char *path);
int         hfs_load                            (HFSVolume *hfs);
int         hfs_close                           (HFSVolume *hfs);

ssize_t     hfs_read                            (HFSVolume *hfs, Buffer *buffer, size_t size, off_t offset);
ssize_t     hfs_read_from_extent                (HFSVolume *hfs, Buffer* buffer, HFSPlusExtentDescriptor *extent, size_t block_size, size_t size, off_t offset);
ssize_t     hfs_readfork                        (HFSFork *fork, Buffer *buffer, size_t size, off_t offset);

ssize_t     hfs_btree_init                      (HFSBTree *tree, HFSFork *fork);
BTreeNode   hfs_btree_get_node                  (HFSBTree *tree, u_int32_t nodeNumber);
u_int16_t   hfs_btree_get_catalog_record_type   (BTreeNode *node, u_int32_t i);

u_int16_t   hfs_btree_get_record_offset         (BTreeNode *node, u_int32_t i);
ssize_t     hfs_btree_get_record_size           (BTreeNode *node, u_int32_t i);
Buffer      hfs_btree_get_record                (BTreeNode *node, u_int32_t i);

// Breaks the record up into a key and value; returns the key length.
ssize_t     hfs_btree_decompose_keyed_record          (const BTreeNode *node, const Buffer *record, Buffer *key, Buffer *value);

#endif
