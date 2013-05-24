//
//  cfile.c
//  hfstest
//
//  Created by Adam Knight on 5/23/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "cfile.h"

typedef struct cfile_meta {
    FILE    *fp;
    char    *path;
    char    *mode;
} cfile_meta;

int init_cfile_stream(STREAM *stream, char* path, char* mode)
{
    if (stream->next != NULL) {
        // We do not support layering.
        errno = EINVAL;
        return -1;
    }
    
    stream->name = path;
    cfile_meta meta = { .path = path, .mode = mode };
    void* meta_heap = malloc(sizeof(cfile_meta));
    memcpy(meta_heap, &meta, sizeof(cfile_meta));
    stream->meta = meta_heap;
    stream->ops = cfile_ops;
    
    return 1;
}

int cfile_open(stream_op_args args)
{
    cfile_meta meta = *((cfile_meta*)(args.stream.meta));
    FILE *fp = fopen(meta.path, meta.mode);
    if (fp != NULL) {
        meta.fp = fp;
        return 1;
    }
    return -1;
}

off_t cfile_seek(stream_op_args args)
{
    return ftello(((cfile_meta*)(args.stream.meta))->fp);
}

int cfile_tell(stream_op_args args)
{
    if (args.op_argc == 1) {
        return fseeko(((cfile_meta*)(args.stream.meta))->fp, *(off_t(*))args.op_argv, SEEK_SET);
    }
    errno = EINVAL;
    return -1;
}

ssize_t cfile_read(stream_op_args args)
{
    if (args.op_argc == 2) {
        FILE    *fp     = ((cfile_meta*)(args.stream.meta))->fp;
        void*   buf     = *(void**)(args.op_argv);
        size_t  length  = (size_t)args.op_argv+sizeof(void*);

        size_t result = fread(buf, 1, length, fp);
        if (result == 0) {
            if (ferror(fp)) {
                clearerr(fp);
                return -1;
            }
            return 0;
        }
        return length;
    }
    
    errno = EINVAL;
    return -1;
}

int cfile_close(stream_op_args args)
{
    return fclose(((cfile_meta*)args.stream.meta)->fp);
}

#define OP u_int64_t(*)(void*)

stream_ops cfile_ops = {
    .open   = cfile_open,
    .seek   = cfile_seek,
    .tell   = cfile_tell,
    .read   = cfile_read,
    .write  = NULL,
    .flush  = NULL,
    .close  = NULL,
    .free   = NULL,
};

