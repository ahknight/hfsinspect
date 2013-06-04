//
//  debug.c
//  hfstest
//
//  Created by Adam Knight on 5/22/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

// http://www.gnu.org/savannah-checkouts/gnu/libc/manual/html_node/Backtraces.html

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

/* Obtain a backtrace and print it to stdout. */
void print_trace (void)
{
    void* callstack[25];
    int size = backtrace(callstack, 25);
    backtrace_symbols_fd(callstack, size, STDERR_FILENO);
}

int stack_depth()
{
    void* stack[40];
    int size = backtrace(stack, 40);
    return size;
}