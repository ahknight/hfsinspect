//
//  prefixstream.c
//  hfsinspect
//
//  Created by Adam Knight on 11/30/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "prefixstream.h"
#include "string.h"

struct prefixstream_context {
    FILE*   fp;
    char    prefix[80];
    uint8_t newline;
};

int prefixstream_close(void * context)
{
    free(context);
    return 0;
}

int prefixstream_write(void * c, const char * buf, int nbytes)
{
    struct prefixstream_context *context = c;
    
    char *token, *string, *tofree;
    
    tofree = string = calloc(1, nbytes+1);
    memcpy(string, buf, nbytes);
    string[nbytes] = '\0';
    assert(string != NULL);
    
    while ( (token = strsep(&string, "\n")) != NULL) {
        
        if (context->newline && strlen(token)) {
            fprintf(context->fp, "%s", (char*)context->prefix);
            context->newline = 0;
        }
        
        fprintf(context->fp, "%s", token);
        
        if ((token && string) || (string != NULL && strlen(string) == 0)) {
            fputc('\n', context->fp);
            context->newline = 1;
        }
    };
    
    free(tofree);
    
    return nbytes;
}

FILE* prefixstream(FILE* fp, char* prefix)
{
    struct prefixstream_context *context = calloc(1, sizeof(struct prefixstream_context));
    context->fp = fp;
    context->newline = 1;
    strlcpy(context->prefix, prefix, 80);
    
    return funopen(context, NULL, prefixstream_write, NULL, prefixstream_close);
}
