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

void swap_BTNodeDescriptor  (BTNodeDescriptor *node);
void swap_BTHeaderRec       (BTHeaderRec *header);
int  swap_BTreeNode         (BTreeNodePtr node);

#endif
