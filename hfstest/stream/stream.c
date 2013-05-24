//
//  stream.c
//  hfstest
//
//  Created by Adam Knight on 5/23/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <stdlib.h>
#include "stream.h"

int stream_open (STREAM stream)
{
    stream_op_args args = {
        .stream = stream,
        .op_name = "open",
        .op_argc = 0,
        .op_argv = NULL,
    };
    return (int)stream.ops.open(args);
}

int stream_tell (STREAM stream, off_t position)
{
    off_t argv[] = { position };
    stream_op_args args = {
        .stream = stream,
        .op_name = "tell",
        .op_argc = 1,
        .op_argv = argv,
    };
    ;
    return (int)stream.ops.tell(args);
}

off_t stream_seek (STREAM stream)
{
    return 0;
}

ssize_t stream_read (STREAM stream, void* buf, size_t length)
{
    return -1;
}

ssize_t stream_write (STREAM stream, void* buf, size_t length)
{
    return -1;
}

int stream_flush (STREAM stream)
{
    return -1;
}

int stream_close (STREAM stream)
{
    return -1;
}

int stream_free (STREAM stream)
{
    return -1;
}
