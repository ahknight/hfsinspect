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

#ifndef	MIN
#define	MIN(a, b)	((a) < (b) ? (a) : (b))
#endif

#ifndef	MAX
#define	MAX(a, b)	((a) > (b) ? (a) : (b))
#endif

#define MAC_GMT_FACTOR      2082844800UL

#pragma Struct Conveniences

HFSFork     make_hfsfork                        (const HFSVolume *hfs, const HFSPlusForkData forkData, u_int8_t forkType, u_int32_t cnid);

int         hfs_open                            (HFSVolume *hfs, const char *path);
int         hfs_load                            (HFSVolume *hfs);
int         hfs_close                           (const HFSVolume *hfs);

ssize_t     hfs_read_blocks                     (Buffer *buffer, const HFSVolume *hfs, u_int32_t block_count, u_int32_t start_block);
ssize_t     hfs_read_from_extent                (Buffer *buffer, const HFSVolume *hfs, const HFSPlusExtentDescriptor *extent, u_int32_t block_count, u_int32_t start_block);
ssize_t     hfs_read_fork                       (Buffer *buffer, const HFSFork *fork, u_int32_t block_count, u_int32_t start_block);
ssize_t     hfs_read_fork_range                 (Buffer *buffer, const HFSFork *fork, size_t size, off_t offset);

ssize_t     hfs_read                            (const HFSVolume *hfs, Buffer *buffer, size_t size, off_t offset);

ssize_t     hfs_btree_init                      (HFSBTree *tree, const HFSFork *fork);

int8_t      hfs_btree_get_node                  (HFSBTreeNode *node, const HFSBTree *tree, u_int32_t nodeNumber);
void        hfs_btree_free_node                 (HFSBTreeNode *node);

off_t       hfs_btree_get_record_offset         (const HFSBTreeNode *node, u_int32_t i);
ssize_t     hfs_btree_get_record_size           (const HFSBTreeNode *node, u_int32_t i);
Buffer      hfs_btree_get_record                (const HFSBTreeNode *node, u_int32_t i);

HFSBTree    hfs_get_catalog_btree               (const HFSVolume *hfs);
HFSBTree    hfs_get_extents_btree               (const HFSVolume *hfs);
HFSBTree    hfs_get_attrbutes_btree             (const HFSVolume *hfs);

u_int16_t   hfs_btree_get_catalog_record_type   (const HFSBTreeNode *node, u_int32_t i);

// Breaks the record up into a key and value; returns the key length.
ssize_t     hfs_btree_decompose_keyed_record    (const HFSBTreeNode *node, const Buffer *record, Buffer *key, Buffer *value);

int8_t      hfs_find_catalog_record             (HFSBTreeNode *node, u_int32_t *recordID, const HFSVolume *hfs, u_int32_t parentFolder, const wchar_t name[256], u_int8_t nameLength);
int8_t      hfs_find_extent_record              (HFSPlusExtentRecord *record, u_int32_t *record_start_block, const HFSFork *fork, u_int32_t startBlock);
u_int8_t    hfs_btree_search_tree               (HFSBTreeNode *node, u_int8_t *index, const HFSBTree *tree, const void *searchKey);
u_int8_t    hfs_btree_search_node               (u_int8_t *index, const HFSBTreeNode *node, const void *searchKey);
int         hfs_btree_compare_catalog_keys      (const HFSPlusCatalogKey *key1, const HFSPlusCatalogKey *key2);
int         hfs_btree_compare_extent_keys       (const HFSPlusExtentKey *key1, const HFSPlusExtentKey *key2);

#endif
