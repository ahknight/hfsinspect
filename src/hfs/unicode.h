//
//  hfs/unicode.h
//  hfsinspect
//
//  Created by Adam Knight on 2015-02-01.
//  Copyright (c) 2015 Adam Knight. All rights reserved.
//

#ifndef hfs_unicode_h
#define hfs_unicode_h

#include <stdint.h>         // uint*
#include "hfs/types.h"      // hfs_str

#pragma mark UTF conversions

int hfsuc_to_str(hfs_str* str, const HFSUniStr255* hfs);
int str_to_hfsuc(HFSUniStr255* hfs, const hfs_str str);

#endif
