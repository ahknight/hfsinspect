//
//  logging.c
//  hfsinspect
//
//  Created by Adam Knight on 5/30/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <libgen.h>
#include <signal.h>         //raise
#include <sys/param.h>      //MIN/MAX

#include <string.h>         // memcpy, strXXX, etc.
#if defined(__linux__)
    #include <bsd/string.h> // strlcpy, etc.
#endif

#include "logging.h"
#include "nullstream.h"
#include "memdmp/output.h"


#define PrintLine_MAX_LINE_LEN 1024
#define PrintLine_MAX_DEPTH    40
#define USE_EMOJI              0

bool DEBUG = false;

struct _colorState {
    struct {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t white;
    } foreground;
    struct {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t white;
    } background;
};
typedef struct _colorState colorState;


void _printColor(FILE* f, unsigned level)
{
    if (_useColor(f) == false) return;

    colorState current;

    switch (level) {
        case L_CRITICAL:
        {
            // Critical: always white on red
            current = (colorState){ { .white = 23 }, { .red = 5 } };
            break;
        }

        case L_ERROR:
        {
            // Error: always red
            current = (colorState){ { .red = 5 }, { 0 } };
            break;
        }

        case L_WARNING:
        {
            // Warning: always orange
            current = (colorState){ { 5, 1, 0}, { 0 } };
            break;
        }

        case L_INFO:
        {
            // Info: shades of green
            current = (colorState){ { 1, 5, 1}, { 0 } };
            break;
        }

        case L_DEBUG:
        {
            // Debug: shades of blue
            current = (colorState){ { 2, 2, 4}, { 0 } };
            break;
        }

        default:
        {
            current = (colorState){ { .white = 23 }, { 0 } };
            break;
        }
    }

    _print_reset(f);
    if (current.foreground.white != 0) {
        _print_gray(f, current.foreground.white, 0);
    } else {
        _print_color(f, current.foreground.red, current.foreground.green, current.foreground.blue, 0);
    }
    if (current.background.white != 0) {
        _print_gray(f, current.background.white, 1);
    } else {
        _print_color(f, current.background.red, current.background.green, current.background.blue, 1);
    }
}

int LogLine(enum LogLevel level, const char* format, ...)
{
    va_list      argp;
    va_start(argp, format);

    int          nchars      = 0;
    FILE*        fp          = stdout;
    static FILE* streams[6]  = {0};
    char*        prefixes[6] = {
#if USE_EMOJI
        "❕",
        "⛔️",
        "🚸 ",
        "🆗 ",
        "ℹ️",
        "🐞 ",
#else
        "[CRIT] ",
        "[ERR] ",
        "[WARN] ",
        "  ",
        "[INFO] ",
        "[DEBUG] ",
#endif
    };

    if (streams[0] == NULL) {
        streams[L_CRITICAL] = stderr;
        streams[L_ERROR]    = stderr;
        streams[L_WARNING]  = stderr;
        streams[L_STANDARD] = stdout;

        if (DEBUG) {
            streams[L_INFO]  = stderr;
            streams[L_DEBUG] = stderr;
        } else {
            streams[L_INFO]  = nullstream();
            streams[L_DEBUG] = nullstream();
        }
    }

    if (level <= L_DEBUG)
        fp = streams[level];
    else
        fprintf(stderr, "Uh oh. %u isn't a valid log level.", level);

    _printColor(fp, level);
    nchars += fputs(prefixes[level], fp);
    nchars += fputs(" ", fp);
    nchars += vfprintf(fp, format, argp);
    fputc('\n', fp);
    _print_reset(fp);

    va_end(argp);

    if ( DEBUG && (level <= L_WARNING) ) {
        print_trace(fp, 2);
    }

    fflush(fp);

    if ((level <= L_CRITICAL)) {
        if (DEBUG)
            raise(SIGTRAP);
        else
            abort();
    }

    return nchars;
}

int PrintLine(enum LogLevel level, const char* file, const char* function, unsigned int line, const char* format, ...)
{
    va_list      argp;
    va_start(argp, format);

    char         inputstr[PrintLine_MAX_LINE_LEN];
    char         indent[PrintLine_MAX_LINE_LEN];
    int          out_bytes = 0;
    unsigned int depth     = stack_depth(PrintLine_MAX_DEPTH) - 1; // remove our frame

    // "Print" the input format string to a string.
    if (argp != NULL)
        vsnprintf((char*)&inputstr, PrintLine_MAX_LINE_LEN, format, argp);
    else
        (void)strlcpy(inputstr, format, PrintLine_MAX_LINE_LEN);

    // Indent the string with the call depth.
    depth = MIN(depth, PrintLine_MAX_DEPTH);

    // Fill the indent string with spaces.
    memset(indent, ' ', depth);

    // Mark the current depth.
    indent[depth] = '\0';

    // Append the text after the indentation.
    (void)strlcat(indent, inputstr, PrintLine_MAX_LINE_LEN);

    // Now add our format to that string and print to stdout.
    if (DEBUG && (level != L_STANDARD))
        out_bytes += LogLine(level, "%-80s [%s:%u %s()]", indent, basename((char*)file), line, function);
    else
        out_bytes += LogLine(level, "%s", inputstr);

    // Clean up.
    va_end(argp);

    return out_bytes;
}

