//
//  hfs_btree.h
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_btree_h
#define hfsinspect_hfs_btree_h

#include "hfs/hfs_types.h"
#include "btree/btree.h"

int hfs_get_btree(BTreePtr *btree, const HFS *hfs, hfs_cnid_t cnid) _NONNULL;

#endif
