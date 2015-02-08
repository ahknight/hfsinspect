//
//  debug.h
//  hfsinspect
//
//  Created by Adam Knight on 5/22/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_debug_h
#define hfsinspect_debug_h

#include <stdio.h>

int print_trace(FILE* fp, unsigned offset) __attribute__((nonnull));
int stack_depth(int max);

#endif
