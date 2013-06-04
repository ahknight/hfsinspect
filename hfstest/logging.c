//
//  logging.c
//  hfstest
//
//  Created by Adam Knight on 5/30/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "logging.h"
#include <malloc/malloc.h>
#include <signal.h>

/*
   log TEXT
!! error TEXT
>> critical TEXT
## debug TEXT                                          [file.c:00;     func()]
 */

void _print_reset(FILE* f)
{
    fprintf(f, "\x1b[0m");
}

void _print_bold(FILE* f)
{
    //(bold?1:22),
    fprintf(f, "\x1b[1m");
}

void _print_gray(FILE* f, u_int8_t gray, bool background)
{
    // values must be from 0-5.
    int color = 232 + gray;
    fprintf(f, "\x1b[%u;5;%um", (background?48:38), color);
}

void _print_color(FILE* f, u_int8_t red, u_int8_t green, u_int8_t blue, bool background)
{
    // values must be from 0-5.
    int color = 16 + (36 * red) + (6 * green) + blue;
    fprintf(f, "\x1b[%u;5;%um", (background?48:38), color);
}

void _print(FILE* f, const char* prefix, const char* file, const char* function, unsigned int line, const char* format, va_list argp)
{
    // "Print" the input format string to a string.
    char* inputstr;
    if (argp != NULL)
        vasprintf(&inputstr, format, argp);
    else
        inputstr = (char*)format;
    
    // Indent the string with the call depth.
    int depth = stack_depth();
    char* indent = malloc(100);
    memset(indent, ' ', malloc_size(indent));
    indent[depth] = '\0';
    strlcat(indent, inputstr, malloc_size(indent));
    
    // Now add our format to that string and print to stdout.
    if (file != NULL)
        fprintf(f, "%s %-80s [%s:%u %s()]\n", prefix, indent, basename((char*)file), line, function);
    else
        fprintf(f, "%s %-s\n", prefix, inputstr);
    _print_reset(f);
    
    // Clean up.
    if (malloc_size(inputstr)) free(inputstr);
    free(indent);
}

void Critical(const char* file, const char* function, int line, const char* format, ...)
{
    va_list argp;
    va_start(argp, format);
    _print_color(stderr, 5, 5, 5, false);
    _print_color(stderr, 5, 0, 0, true);
    _print(stderr, ">>", basename((char*)file), function, line, format, argp);
    _print_reset(stderr);
    va_end(argp);
}

void Error(const char* file, const char* function, int line, const char* format, ...)
{
    va_list argp;
    va_start(argp, format);
    _print_color(stderr, 5, 0, 0, false);
    _print(stderr, "!!", basename((char*)file), function, line, format, argp);
    _print_reset(stderr);
    va_end(argp);
}

void Print(const char* format, ...)
{
    va_list argp;
    va_start(argp, format);
    _print(stdout, " ", NULL, NULL, 0, format, argp);
    _print_reset(stderr);
    va_end(argp);
}

void Info(const char* file, const char* function, int line, const char* format, ...)
{
    va_list argp;
    va_start(argp, format);
//    _print_bold(stderr);
    _print_color(stderr, 1, 5, 1, false);
    _print(stderr, "##", basename((char*)file), function, line, format, argp);
    _print_reset(stderr);
    va_end(argp);
}

void Debug(const char* file, const char* function, int line, const char* format, ...)
{
    va_list argp;
    va_start(argp, format);
    _print_color(stderr, 2, 2, 4, false);
    _print(stderr, "] ", basename((char*)file), function, line, format, argp);
    _print_reset(stderr);
    va_end(argp);
}
