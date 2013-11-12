//
//  hfs_attribute_ops.h
//  hfsinspect
//
//  Created by Adam Knight on 6/16/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_attribute_ops_h
#define hfsinspect_hfs_attribute_ops_h

#include "hfs_structs.h"

HFSBTree    hfs_get_attribute_btree     (const HFSVolume *hfs);
int         hfs_attributes_compare_keys (const HFSPlusAttrKey *key1, const HFSPlusAttrKey *key2);

#endif
