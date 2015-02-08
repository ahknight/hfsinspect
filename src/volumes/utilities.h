//
//  utilities.h
//  volumes
//
//  Created by Adam Knight on 11/26/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef volumes_utilities_h
#define volumes_utilities_h

#include <stdlib.h>
#include <stdio.h>

ssize_t fpread(FILE* f, void* buf, size_t nbytes, off_t offset) __attribute__((nonnull(1,2)));

#endif
