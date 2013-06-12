//
//  hfs_extents.h
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_extents_h
#define hfsinspect_hfs_extents_h

#include "hfs_structs.h"
#include "hfs_extentlist.h"

HFSBTree    hfs_get_extents_btree       (const HFSVolume *hfs);
int8_t      hfs_extents_find_record     (HFSPlusExtentRecord *record, hfs_block *record_start_block, const HFSFork *fork, size_t startBlock);
int         hfs_extents_compare_keys    (const HFSPlusExtentKey *key1, const HFSPlusExtentKey *key2);
bool        hfs_extents_get_extentlist_for_fork(ExtentList* list, const HFSFork* fork);

#endif
