//
//  hotfiles.h
//  hfsinspect
//
//  Created by Adam Knight on 12/3/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hotfiles_h
#define hfsinspect_hotfiles_h

#include "hfs/btree/btree.h"
#include "hfs/hfs_structs.h"

int hfs_get_hotfiles_btree(BTreePtr *tree, const HFS *hfs);
int hfs_hotfiles_get_node(BTreeNodePtr *node, const BTreePtr bTree, bt_nodeid_t nodeNum);

#endif
