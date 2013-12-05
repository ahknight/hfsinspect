//
//  btree_endian.h
//  hfsinspect
//
//  Created by Adam Knight on 11/26/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_btree_endian_h
#define hfsinspect_btree_endian_h

#include "hfs/btree/btree.h"

void swap_BTNodeDescriptor  (BTNodeDescriptor *node) _NONNULL;
void swap_BTHeaderRec       (BTHeaderRec *header) _NONNULL;
int  swap_BTreeNode         (BTreeNodePtr node) _NONNULL;

#endif
