//
//  logging.c
//  hfsinspect
//
//  Created by Adam Knight on 5/30/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "logging.h"
#include <stdarg.h>
#include <libgen.h>
#include <sys/param.h>  //MIN/MAX

struct _colorState {
    struct {
        uint8_t    red;
        uint8_t    green;
        uint8_t    blue;
        uint8_t    white;
    }           foreground;
    struct {
        uint8_t    red;
        uint8_t    green;
        uint8_t    blue;
        uint8_t    white;
    }           background;
};
typedef struct _colorState colorState;

bool _useColor()
{
    return ( getenv("NOCOLOR") == NULL );
}

void _print_reset(FILE* f)
{
    if (_useColor() == false) return;
    fprintf(f, "\x1b[0m");
}

void _print_gray(FILE* f, uint8_t gray, bool background)
{
    if (_useColor() == false) return;
    // values must be from 0-23.
    int color = 232 + gray;
    fprintf(f, "\x1b[%u;5;%um", (background?48:38), color);
}

void _print_color(FILE* f, uint8_t red, uint8_t green, uint8_t blue, bool background)
{
    if (_useColor() == false) return;
    // values must be from 0-5.
    if (red == green && green == blue) {
        _print_gray(f, red, background);
        return;
    }
    
    int color = 16 + (36 * red) + (6 * green) + blue;
    fprintf(f, "\x1b[%u;5;%um", (background?48:38), color);
}

void _printColor(FILE* f, unsigned level)
{
    if (_useColor() == false) return;

    // Critical:    always white on red
    colorState critical = { { .white = 23 }, { .red = 5 } };
    
    // Error:       always red
    colorState error    = { { .red = 5 }, { 0 } };
    
    // Warning:     always orange
    colorState warning  = { { 5, 1, 0}, { 0 } };
    
    // Info:        shades of green
    colorState info     = { { 1, 5, 1}, { 0 } };
    
    // Debug:       shades of blue
    colorState debug    = { { 2, 2, 4}, { 0 } };
    
    //    const int max_depth = 15;
    //
    //    int depth = stack_depth(max_depth);
    //
    //    float colorMap[max_depth][3] = {
    //        {1, 1, 1}, //0 - start()
    //        {1, 1, 1}, //1 - main()
    //        {1, 1, 5}, //2 - calls from main()
    //        {1, 2, 1}, //3 - functions or libraries
    //        {1, 3, 1},
    //
    //        {1, 4, 1}, //5
    //        {1, 5, 1},
    //        {1, 1, 2},
    //        {1, 1, 3},
    //        {1, 1, 4},
    //
    //        {1, 1, 5}, //10
    //        {2, 1, 1},
    //        {3, 1, 1},
    //        {4, 1, 1},
    //        {5, 1, 1},
    //    };
    
    colorState current = { { .white = 23 }, { 0 } };
    switch (level) {
        case L_CRITICAL:
            current = critical;
            break;
            
        case L_ERROR:
            current = error;
            break;
            
        case L_WARNING:
            current = warning;
            break;
            
        case L_INFO:
            current = info;
            break;
            
        case L_DEBUG:
            current = debug;
            break;
            
        default:
            break;
    }
    
    _print_reset(f);
    if (current.foreground.white) {
        _print_gray(f, current.foreground.white, 0);
    } else {
        _print_color(f, current.foreground.red, current.foreground.green, current.foreground.blue, 0);
    }
    if (current.background.white) {
        _print_gray(f, current.background.white, 1);
    } else {
        _print_color(f, current.background.red, current.background.green, current.background.blue, 1);
    }
}

void PrintLine(FILE* f, enum LogLevel level, const char* file, const char* function, unsigned int line, const char* format, ...)
{
    va_list argp;
    va_start(argp, format);
    
    // "Print" the input format string to a string.
    char inputstr[1024];
    if (argp != NULL)
        vsnprintf((char*)&inputstr, 1024, format, argp);
    else
        strlcpy(inputstr, format, 1024);
    
    // Indent the string with the call depth.
    unsigned int depth = stack_depth(40) - 1; // remove our frame
    depth = MIN(depth, 40);
    char indent[1024];
    
    // Fill the indent string with spaces.
    memset(indent, ' ', 1024);
    
    // Mark the current depth.
    indent[depth] = '\0';
    
    // Append the text after the indentation.
    strlcat(indent, inputstr, 1024);
    
    bool showLineInfo = (getenv("DEBUG") != 0);
    char* levelName;
    
    switch (level) {
        case L_CRITICAL:
            levelName = "CRIT";
            break;

        case L_ERROR:
            levelName = "ERROR";
            break;

        case L_WARNING:
            levelName = "WARN";
            break;

        case L_STANDARD:
            levelName = "OUT";
            break;

        case L_INFO:
            levelName = "INFO";
            break;

        case L_DEBUG:
            levelName = "DEBUG";
            break;

        default:
            break;
    }
    
    // Now add our format to that string and print to stdout.
    _printColor(f, level);
    if (file != NULL && showLineInfo)
        fprintf(f, "[%2d] [%-5s]%-80s [%s:%u %s()]\n", depth, levelName, indent, basename((char*)file), line, function);
    else
        fprintf(f, "%s\n", inputstr);
    _print_reset(f);
    
    // Clean up.
    va_end(argp);
}
