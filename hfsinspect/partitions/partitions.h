//
//  partitions.h
//  hfsinspect
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs_structs.h"

#ifndef hfsinspect_partitions_h
#define hfsinspect_partitions_h

#pragma mark - Structures

#include "mbr.h"
#include "gpt.h"
#include "apm.h"
#include "corestorage.h"

#pragma mark - Functions

bool sniff_and_print(HFSVolume* hfs);

#endif
