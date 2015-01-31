//
//  volumes.h
//  volumes
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef volumes_h
#define volumes_h

#pragma mark - Structures

#include "volume.h"
#include "mbr.h"
#include "gpt.h"
#include "apm.h"
#include "corestorage.h"

#pragma mark - Functions

int volumes_load (Volume *vol) __attribute__((nonnull));
int volumes_dump (Volume *vol) __attribute__((nonnull));

#endif
