//
//  hfs.h
//  hfsinspect
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_h
#define hfsinspect_hfs_h

#include "hfs_structs.h"
#include "hfs_endian.h"
#include "hfs_io.h"
#include "hfs_btree.h"
#include "hfs_catalog_ops.h"
#include "hfs_extent_ops.h"
#include "hfs_extentlist.h"
#include "hfs_pstruct.h"

#pragma mark Struct Conveniences

int hfs_open    (HFSVolume *hfs, const char *path);
int hfs_load    (HFSVolume *hfs);
int hfs_close   (HFSVolume *hfs);

bool hfs_get_HFSPlusVolumeHeader    (HFSPlusVolumeHeader* vh, const HFSVolume* hfs);
bool hfs_get_JournalInfoBlock       (JournalInfoBlock* block, const HFSVolume* hfs);

#endif
