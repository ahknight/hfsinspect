//
//  logging.h
//  hfstest
//
//  Created by Adam Knight on 5/30/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfstest_logging_h
#define hfstest_logging_h

#include "debug.h"

#include <signal.h>
#include <stdbool.h>

#define out(...)        Print(__VA_ARGS__)
#define error(...)      Error(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define critical(...)   { Critical(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__); if (getenv("DEBUG")) { print_trace(); raise(SIGTRAP); } else { exit(1); }; }
#define info(...)       if (getenv("DEBUG")) { Info(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__); }
#define debug(...)      if (getenv("DEBUG")) { Debug(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__); }

void _print_reset(FILE* f);
void _print_color(FILE* f, u_int8_t red, u_int8_t green, u_int8_t blue, bool background);

void Print(const char* format, ...);
void Error(const char* file, const char* function, int line, const char* format, ...);
void Critical(const char* file, const char* function, int line, const char* format, ...);
void Info(const char* file, const char* function, int line, const char* format, ...);
void Debug(const char* file, const char* function, int line, const char* format, ...);

#endif
