//
//  logging.h
//  hfsinspect
//
//  Created by Adam Knight on 5/30/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_logging_h
#define hfsinspect_logging_h

#include "debug.h"      //print_trace
#include <stdio.h>      //FILE*
#include <stdint.h>     //uint#_t etc.
#include <stdbool.h>    //bool
#include <signal.h>     //raise

void critical(char* format, ...) __attribute((format(printf,1,2), noreturn));
#define critical(...)   PrintLine(L_CRITICAL, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);

void error(char* format, ...) __attribute((format(printf,1,2)));
#define error(...)      PrintLine(L_ERROR, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

void warning(char* format, ...) __attribute((format(printf,1,2)));
#define warning(...)    PrintLine(L_WARNING, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

void print(char* format, ...) __attribute((format(printf,1,2)));
#define print(...)      PrintLine(L_STANDARD, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

void info(char* format, ...) __attribute((format(printf,1,2)));
#define info(...)       if (DEBUG) { PrintLine(L_INFO, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__); }

void debug(char* format, ...) __attribute((format(printf,1,2)));
#define debug(...)      if (DEBUG) { PrintLine(L_DEBUG, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__); }

void fatal(char* format, ...) __attribute((format(printf,1,2), noreturn));
#define fatal(...)      { fprintf(stderr, __VA_ARGS__); fputc('\n', stdout); usage(1); }

extern bool DEBUG;

enum LogLevel {
    L_CRITICAL = 0,
    L_ERROR,
    L_WARNING,
    L_STANDARD,
    L_INFO,
    L_DEBUG
};

void _print_reset(FILE* f);
void _print_gray(FILE* f, uint8_t gray, bool background);
void _print_color(FILE* f, uint8_t red, uint8_t green, uint8_t blue, bool background);

int LogLine(enum LogLevel level, const char* format, ...)
    __attribute__((format(printf, 2, 3)));
int PrintLine(enum LogLevel level, const char* file, const char* function, unsigned int line, const char* format, ...)
    __attribute__((format(printf, 5, 6)));

#endif
