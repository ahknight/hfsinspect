//
//  hfs_btree.h
//  hfsinspect
//
//  Created by Adam Knight on 6/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_btree_h
#define hfsinspect_hfs_btree_h

#include "hfs/types.h"
#include "hfs/btree/btree.h"

int hfs_get_btree(BTreePtr* btree, const HFSPlus* hfs, hfs_cnid_t cnid) __attribute__((nonnull));

#endif
