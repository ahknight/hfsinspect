//
//  debug.c
//  hfsinspect
//
//  Created by Adam Knight on 5/22/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

// http://www.gnu.org/savannah-checkouts/gnu/libc/manual/html_node/Backtraces.html

#include "debug.h"

#include <stdlib.h>
#include <stdio.h>
#include <execinfo.h>

int print_trace(FILE* fp, unsigned offset)
{
    int    nbytes = 0;
    void*  stack[128];
    int    size   = 0;
    char** lines  = NULL;

    offset += 1; // Omit this function.
    size    = backtrace(stack, 128);
    lines   = backtrace_symbols(&stack[offset], size - offset);
    for (int i = 0; i < (size - offset); i++) {
        nbytes += fprintf(fp, "%s\n", lines[i]);
    }
    SFREE(lines);
    return nbytes;
}

int stack_depth(int max)
{
    void* stack[max];
    int   size = backtrace(stack, max);
    return size - 1; // Account for stack_depth's frame
}

