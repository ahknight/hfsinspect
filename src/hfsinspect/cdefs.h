//
//  cdefs.h
//  hfsinspect
//
//  Created by Adam Knight on 11/10/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_cdefs_h
#define hfsinspect_cdefs_h

#include <assert.h>             // assert

// Tired of typing this? Me too.
#ifndef FOR_UNTIL
#define FOR_UNTIL(var, end) for(int var = 0; var < end; var++)
#endif

// This, too.
#ifndef ALLOC
#define ALLOC(name, size) { (name) = calloc(1, (size)); assert((name) != NULL); }
#endif

// Also this.
#ifndef FREE
#define FREE(name) { free((name)); (name) = NULL; }
#endif

#endif
