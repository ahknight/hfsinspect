//
//  cdefs.h
//  hfsinspect
//
//  Created by Adam Knight on 11/10/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_cdefs_h
#define hfsinspect_cdefs_h


#pragma mark - ANSI C

#include <stdlib.h>             // malloc, free
#include <stdint.h>             // uint*
#include <stdbool.h>            // bool, true, false
#include <math.h>               // log and friends
#include <stdio.h>              // FILE*
#include <wchar.h>
#include <assert.h>             // assert
#include <errno.h>              // errno/perror
#include <string.h>             // memcpy, strXXX, etc.
#if defined(__linux__)
#include <bsd/string.h>         // strlcpy, etc.
#endif


#pragma mark - POSIX

#include <unistd.h>


#pragma mark - BSD

#include <sys/param.h>          // MIN/MAX
#include <sys/stat.h>           // stat, statfs


#include <uuid/uuid.h>
#ifndef _UUID_STRING_T
#define _UUID_STRING_T
typedef char uuid_string_t[37];
#endif /* _UUID_STRING_T */


#pragma mark - Memory Management

#if defined(GC_ENABLED)         // GC_ENABLED

#include "gc.h"

#define ALLOC(buf_size)        GC_MALLOC((buf_size))
#define REALLOC(buf, buf_size) GC_REALLOC((buf), (buf_size))
#define FREE(buf)              ;;

#else                           // !GC_ENABLED

#define ALLOC(buf_size)        calloc(1, (buf_size))
#define REALLOC(buf, buf_size) realloc((buf), (buf_size))
#define FREE(buf)              free((buf))

#endif                          // defined(GC_ENABLED)

#define SALLOC(buf, buf_size)   { (buf) = ALLOC((buf_size)); assert((buf) != NULL); }
#define SREALLOC(buf, buf_size) { (buf) = REALLOC((buf), (buf_size)); assert((buf) != NULL); }
#define SFREE(buf)              { FREE((buf)); (buf) = NULL; }

#endif  // hfsinspect_cdefs_h
