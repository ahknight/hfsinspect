//
//  hfs_extents.h
//  hfstest
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfstest_hfs_extents_h
#define hfstest_hfs_extents_h

#include "hfs_structs.h"

HFSBTree    hfs_get_extents_btree       (const HFSVolume *hfs);
int8_t      hfs_extents_find_record     (HFSPlusExtentRecord *record, size_t *record_start_block, const HFSFork *fork, size_t startBlock);
int         hfs_extents_compare_keys    (const HFSPlusExtentKey *key1, const HFSPlusExtentKey *key2);

#endif
