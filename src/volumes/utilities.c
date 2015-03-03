//
//  utilities.c
//  volumes
//
//  Created by Adam Knight on 11/26/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "utilities.h"

ssize_t fpread(FILE* f, void* buf, size_t nbytes, off_t offset)
{
    if ( fseeko(f, offset, SEEK_SET) == 0 )
        return fread(buf, 1, nbytes, f);
    else
        return -1;
}

