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

int         hfs_open                            (HFSVolume *hfs, const char *path);
int         hfs_load                            (HFSVolume *hfs);
int         hfs_close                           (HFSVolume *hfs);

ssize_t     hfs_read                            (HFSVolume *hfs, char *buf, size_t size, off_t offset);
ssize_t     hfs_read_from_extent                (int fd, char* buffer, HFSPlusExtentDescriptor *extent, size_t block_size, size_t size, off_t offset);
ssize_t     hfs_readfork                        (HFSFork *fork, char *buf, size_t size, off_t offset);

ssize_t     hfs_btree_init                      (HFSBTree *tree, HFSFork *fork);
ssize_t     hfs_btree_read_node                 (HFSBTree *tree, BTreeNode *node, u_int32_t nodeNumber);
u_int16_t   hfs_btree_get_catalog_record_type   (BTreeNode *node, u_int32_t i);

u_int16_t   hfs_btree_get_record_offset         (BTreeNode *node, u_int32_t i);
ssize_t     hfs_btree_get_record_size           (BTreeNode *node, u_int32_t i);
ssize_t     hfs_btree_get_record                (BTreeNode *node, u_int32_t i, char* buffer);
ssize_t     hfs_btree_decompose_record          (const char* record, const size_t record_size, BTreeKey *key, u_int16_t *key_length, char* value, size_t *value_size);

#endif
