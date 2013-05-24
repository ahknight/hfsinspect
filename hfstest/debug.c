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
    void    *array[10];
    size_t  size;
    char    **strings;
    size_t  i;
    
    size    = backtrace (array, 10);
    strings = backtrace_symbols(array, (int)size);
    
    printf("Obtained %zd stack frames.\n", size);
    
    for (i = 0; i < size; i++)
        printf("%s\n", strings[i]);
    
    free(strings);
}