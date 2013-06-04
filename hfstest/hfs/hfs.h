//
//  hfs.h
//  hfstest
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfstest_hfs_h
#define hfstest_hfs_h

#include "hfs_structs.h"
#include "hfs_endian.h"
#include "hfs_io.h"
#include "hfs_btree.h"
#include "hfs_catalog_ops.h"
#include "hfs_extent_ops.h"
#include "hfs_pstruct.h"

#pragma Struct Conveniences

int hfs_open    (HFSVolume *hfs, const char *path);
int hfs_load    (HFSVolume *hfs);
int hfs_close   (const HFSVolume *hfs);

#endif
