//
//  cdefs.h
//  hfsinspect
//
//  Created by Adam Knight on 11/10/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_cdefs_h
#define hfsinspect_cdefs_h

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

//#include <assert.h>             // assert()
#define _assert( condition ) { if ( ! (condition) ) { fprintf(stderr, "%s:%d assertation failed: %s", __FILE__, __LINE__, #condition); exit(1); } }
#define assert( condition ) _assert( (condition) )

// Tired of typing this? Me too.
#ifndef FOR_UNTIL
#define FOR_UNTIL(var, end) for(int var = 0; var < end; var++)
#endif

// This, too.
#ifndef ALLOC
#define ALLOC(name, size) { (name) = calloc((size), 1); if ((name) == NULL) { perror("calloc"); exit(errno); }; }
#endif

// Also this.
#ifndef FREE
#define FREE(name) { free((name)); (name) = NULL; }
#endif

#ifndef _UUID_STRING_T
#define _UUID_STRING_T
typedef char	uuid_string_t[37];
#endif /* _UUID_STRING_T */

#endif
