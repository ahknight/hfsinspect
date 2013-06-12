//
//  debug.c
//  hfsinspect
//
//  Created by Adam Knight on 5/22/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

// http://www.gnu.org/savannah-checkouts/gnu/libc/manual/html_node/Backtraces.html

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

/* Obtain a backtrace and print it to stdout. */
void print_trace (char* comment, ...)
{
    va_list args;
    va_start(args, comment);
    
    if (comment != NULL) { vfprintf(stderr, comment, args); }
    
    void* stack[100];
    int size = backtrace(stack, 100);
    backtrace_symbols_fd(&stack[1], size - 1, STDERR_FILENO);
    
    va_end(args);
}

int stack_depth(int max)
{
    void* stack[max];
    int size = backtrace(stack, max);
    return size - 1; // Account for stack_depth's frame
}