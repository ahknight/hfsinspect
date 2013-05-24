//
//  stream.h
//  hfstest
//
//  Created by Adam Knight on 5/23/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfstest_stream_h
#define hfstest_stream_h

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

/* Declarations */
struct STREAM;
typedef struct STREAM STREAM;

struct stream_ops;
typedef struct stream_ops stream_ops;

struct stream_op_args;
typedef struct stream_op_args stream_op_args;

/* Functions and Function Pointer Typedefs */

typedef ssize_t(*uio_read)(void* cookie, void *buf, size_t nbyte);
typedef ssize_t(*uio_write)(void* cookie, void *buf, size_t nbyte);
typedef off_t(*uio_seek)(void* cookie, off_t offset, int whence);
typedef int(*uio_close)(void* cookie);



int     stream_open     (STREAM stream);
typedef int(*stream_open_func)(stream_op_args);

int     stream_tell     (STREAM stream, off_t position);
typedef int(*stream_tell_func)(stream_op_args);

off_t   stream_seek     (STREAM stream);
typedef off_t(*stream_seek_func)(stream_op_args);

ssize_t stream_read     (STREAM stream, void* buf, size_t length);
typedef ssize_t(*stream_read_func)(stream_op_args);

ssize_t stream_write    (STREAM stream, void* buf, size_t length);
typedef ssize_t(*stream_write_func)(stream_op_args);

int     stream_flush    (STREAM stream);
typedef int(*stream_flush_func)(stream_op_args);

int     stream_close    (STREAM stream);
typedef int(*stream_close_func)(stream_op_args);

int     stream_free     (STREAM stream);
typedef int(*stream_free_func)(stream_op_args);


/* Structs */
struct stream_ops {
    stream_open_func    open;
    stream_seek_func    seek;
    stream_tell_func    tell;
    stream_read_func    read;
    stream_write_func   write;
    stream_flush_func   flush;
    stream_close_func   close;
    stream_free_func    free;
};

struct STREAM {
    char*   name;       // A name that is useful for debugging
    void*   meta;       // Implementation-specific metadata
    struct stream_ops ops;  // Function pointers for stream ops
    size_t  size;       // Logical (eg. without compression)
    size_t  length;     // Literal (eg. size on disk)
    bool    seekable;   // Character v. block
    bool    local;      // True if this stream is on the local machine.
    struct STREAM *next;    // Next stream in the chain
};

struct stream_op_args {
    STREAM  stream;     // The target stream
    char*   op_name;    // Which operation is being performed
    int     op_argc;    // Count of arguments
    void*   op_argv;    // Array of arguments, op_argc in length
};

#endif
