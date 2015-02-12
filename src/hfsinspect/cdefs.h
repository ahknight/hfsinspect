//
//  cdefs.h
//  hfsinspect
//
//  Created by Adam Knight on 11/10/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_cdefs_h
#define hfsinspect_cdefs_h

#include <stdlib.h>             // malloc, free
#include <assert.h>             // assert

#ifndef ALLOC
#define ALLOC(name, size) { (name) = calloc(1, (size)); assert((name) != NULL); }
#endif

#ifndef FREE
#define FREE(name) { free((name)); (name) = NULL; }
#endif

#endif
