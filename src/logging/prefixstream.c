//
//  prefixstream.c
//  hfsinspect
//
//  Created by Adam Knight on 11/30/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "prefixstream.h"

#include <stdlib.h>
#include <stdint.h>
#include <sys/param.h>
#include <assert.h>             // assert()

#include <string.h>             // memcpy, strXXX, etc.
#if defined(__linux__)
    #include <bsd/string.h>     // strlcpy, etc.
#endif

#ifndef ALLOC
#define ALLOC(name, size) { (name) = calloc(1, (size)); assert((name) != NULL); }
#endif

struct ps_ctx {
    FILE*   fp;
    char    prefix[80];
    uint8_t newline;
};
typedef struct ps_ctx ps_ctx;

int prefixstream_close(void* context)
{
    free(context);
    return 0;
}

#if defined (BSD)
int prefixstream_write(void* c, const char* buf, int nbytes)
#else
ssize_t prefixstream_write(void* c, const char* buf, size_t nbytes)
#endif
{
    ps_ctx* context = c;
    char*   token   = NULL;
    char*   string  = NULL;
    char*   tofree  = NULL;

    tofree         = string = calloc(1, nbytes+1);
    memcpy(string, buf, nbytes);
    string[nbytes] = '\0';
    assert(string != NULL);
    assert(context->fp);

    // We don't always get a single line, so split on new lines
    // and only output a prefix for each line.
    while ((token = strsep(&string, "\n")) != NULL) {

        fputs((char*)context->prefix, context->fp);
        fputs(token, context->fp);
        fputc('\n', context->fp);

        // If we have a zero-length token then that's a newline.
//        if ( string && (strlen(string) == 0) ) {
//            fputc('\n', context->fp);
//            context->newline = 1;
//        }
    }

    free(tofree);

    return nbytes;
}

FILE* prefixstream(FILE* fp, char* prefix)
{
    ps_ctx* context = NULL;
    ALLOC(context, sizeof(ps_ctx));
    context->fp      = fp;
    context->newline = 1;
    (void)strlcpy(context->prefix, prefix, 80);

#if defined(BSD)
    return funopen(context, NULL, prefixstream_write, NULL, prefixstream_close);
#else
    cookie_io_functions_t prefix_funcs = {
        NULL,
        prefixstream_write,
        NULL,
        prefixstream_close
    };
    return fopencookie(context, "w", prefix_funcs);
#endif
}

