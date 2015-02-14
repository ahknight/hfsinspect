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

#if defined(GC_ENABLED)         // GC_ENABLED

#include "gc.h"

#define ALLOC(buf_size)         GC_MALLOC((buf_size))
#define REALLOC(buf, buf_size)  GC_REALLOC((buf), (buf_size))
#define FREE(buf)               ;;

#else                           // !GC_ENABLED

#define ALLOC(buf_size)         calloc(1, (buf_size))
#define REALLOC(buf, buf_size)  realloc((buf), (buf_size))
#define FREE(buf)               free((buf))

#endif                          // defined(GC_ENABLED)

#define SALLOC(buf, buf_size)   { (buf) = ALLOC((buf_size)); assert((buf) != NULL); }
#define SREALLOC(buf, buf_size) { (buf) = REALLOC((buf), (buf_size)); assert((buf) != NULL); }
#define SFREE(buf)              { FREE((buf)); (buf) = NULL; }

#endif  // hfsinspect_cdefs_h
