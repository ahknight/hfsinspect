//
//  logging.h
//  hfsinspect
//
//  Created by Adam Knight on 5/30/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_logging_h
#define hfsinspect_logging_h

#include <stdint.h>     //uint#_t etc.
#include <stdio.h>      //FILE*
#include <stdbool.h>    //bool
#include "debug.h"      //print_trace

/*
   The functions aren't actually defined; the preprocessor fills them in.  They exist
   for the compiler attributes.
 */

void critical(char* format, ...) __attribute((format(printf,1,2), noreturn));
#define critical(...) { PrintLine(L_CRITICAL, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__); assert(1==0); }

void error(char* format, ...) __attribute((format(printf,1,2)));
#define error(...) PrintLine(L_ERROR, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

void warning(char* format, ...) __attribute((format(printf,1,2)));
#define warning(...) PrintLine(L_WARNING, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

void print(char* format, ...) __attribute((format(printf,1,2)));
#define print(...) PrintLine(L_STANDARD, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

void info(char* format, ...) __attribute((format(printf,1,2)));
#define info(...) PrintLine(L_INFO, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);

void debug(char* format, ...) __attribute((format(printf,1,2)));
#define debug(...) PrintLine(L_DEBUG, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);

void debug2(char* format, ...) __attribute((format(printf,1,2)));
#define debug2(...) PrintLine(L_DEBUG2, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);

void trace(char* format, ...) __attribute((format(printf,1,2)));
#define trace(...) PrintLine(L_TRACE, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);

enum LogLevel {
    L_CRITICAL = 0,
    L_ERROR    = 1,
    L_WARNING  = 2,
    L_STANDARD = 3,
    L_INFO     = 4,
    L_DEBUG    = 5,
    L_DEBUG2   = 6,
    L_TRACE    = 7,
};

extern enum LogLevel log_level;

int LogLine(enum LogLevel level, const char* format, ...)
__attribute__((format(printf, 2, 3)));

int PrintLine(enum LogLevel level, const char* file, const char* function, unsigned int line, const char* format, ...)
__attribute__((format(printf, 5, 6), nonnull));

#endif
