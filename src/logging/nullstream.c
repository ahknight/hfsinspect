//
//  nullstream.c
//  hfsinspect
//
//  Created by Adam Knight on 5/30/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "nullstream.h"
#include <sys/param.h>

#if defined(BSD)

fpos_t nullstream_seek(void* context, fpos_t pos, int whence) { return 0; }
int nullstream_read(void* context, char* buf, int nbytes) { return nbytes; }
int nullstream_write(void* context, const char* buf, int nbytes) { return nbytes; }

FILE* nullstream()
{
    static FILE* np = NULL;
    if (np == NULL) np = funopen(NULL, nullstream_read, nullstream_write, nullstream_seek, NULL);
    return np;
}

#else

int nullstream_seek(void* context, off_t* pos, int whence) { return 0; }
ssize_t nullstream_read(void* context, char* buf, size_t nbytes) { return nbytes; }
ssize_t nullstream_write(void* context, const char* buf, size_t nbytes) { return nbytes; }

FILE* nullstream()
{
    static FILE* np = NULL;
    if (np == NULL) {
        cookie_io_functions_t null_funcs = {
            nullstream_read,
            nullstream_write,
            nullstream_seek,
            NULL
        };
        np = fopencookie(NULL, "w+", null_funcs);
    }
    return np;
}

#endif
