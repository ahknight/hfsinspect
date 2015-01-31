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

void swap_BTNodeDescriptor  (BTNodeDescriptor *node) __attribute__((nonnull));
void swap_BTHeaderRec       (BTHeaderRec *header) __attribute__((nonnull));
int  swap_BTreeNode         (BTreeNodePtr node) __attribute__((nonnull));

#endif
