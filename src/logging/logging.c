//
//  logging.c
//  hfsinspect
//
//  Created by Adam Knight on 5/30/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <libgen.h>
#include <signal.h>         // raise
#include <sys/param.h>      // MIN/MAX
#include <sys/ioctl.h>      // ioctl

#include "logging.h"
#include "memdmp/output.h"


#define PrintLine_MAX_LINE_LEN 0xffff
#define PrintLine_MAX_DEPTH    40
#define USE_EMOJI              0

enum LogLevel log_level = L_STANDARD;

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

        case L_STANDARD:
        {
            current = (colorState){ { .white = 23 }, { 0 } };
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

        case L_DEBUG2:
        {
            // Debug: shades of blue
            current = (colorState){ { 3, 3, 5}, { 0 } };
            break;
        }

        default:
        {
            current = (colorState){ { 5, 5, 1 }, { 0 } };
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
    va_list argp;
    va_start(argp, format);

    int     nchars      = 0;
    FILE*   fp          = stderr;
    char*   prefixes[8] = {
#if USE_EMOJI
        "❕",
        "⛔️",
        "🚸 ",
        "🆗 ",
        "ℹ️ ",
        "🐞 ",
        "🐞 ",
        "🐞 ",
#else
        "[CRIT]   ",
        "[ERR]    ",
        "[WARN]   ",
        "",
        "[INFO]   ",
        "[DEBUG]  ",
        "[DEBUG2] ",
        "[TRACE]  ",
#endif
    };
    char*   prefix = NULL;

    if (level > log_level)   return 0;

    if (level == L_STANDARD) fp = stdout;

    if (level >= L_TRACE) {
        prefix = prefixes[L_TRACE];
    } else {
        prefix = prefixes[level];
    }

    _printColor(fp, level);
    nchars += fputs(prefix, fp);
    nchars += fputs(" ", fp);
    nchars += vfprintf(fp, format, argp);
    nchars += fputc('\n', fp);
    _print_reset(fp);

    va_end(argp);

    if ( (log_level > L_INFO) && (level <= L_WARNING) ) {
        print_trace(fp, 2);
    }

    fflush(fp);

    if ((level <= L_CRITICAL)) {
        if (log_level > L_INFO)
            raise(SIGTRAP);
        else
            abort();
    }

    return nchars;
}

int PrintLine(enum LogLevel level, const char* file, const char* function, unsigned int line_no, const char* format, ...)
{
    va_list        argp;
    va_start(argp, format);

    unsigned short term_width = 0;
    struct winsize w          = {0};
    int            fd         = fileno(stdout);
    char*          in_line    = NULL;
    char*          out_line   = NULL;
    int            out_bytes  = 0;
    unsigned int   depth      = stack_depth(PrintLine_MAX_DEPTH) - 1; // remove our frame

    // Get the terminal's line width.
    if (isatty(fd)) {
        ioctl(fd, TIOCGWINSZ, &w);
        term_width = w.ws_col;
    } else {
        term_width = PrintLine_MAX_LINE_LEN;
    }

    term_width -= 10; // Account for prefix.

    SALLOC(out_line, term_width);
    SALLOC(in_line, term_width);

    // Fill the line with spaces.
    memset(out_line, ' ', term_width);

    // Indent the string with the call depth.
    depth = MIN(depth, term_width);

    // "Print" the input format string to a string.
    unsigned in_line_len = 0;
//    if (argp != NULL)
    in_line_len = vsnprintf(in_line, term_width, format, argp);
//    else
//        in_line_len = strlcpy(in_line, format, term_width);

    // Append the text after the indentation.
    (void)memcpy((char*)(out_line+depth), in_line, in_line_len);

    // Now add our format to that string and print to stdout.
    if ((log_level > L_INFO) && (level != L_STANDARD)) {
        char*  debug_info   = NULL;
        size_t debug_len    = 0;
        int    debug_offset = 0;

        debug_info   = ALLOC(term_width);
        debug_len    = snprintf(debug_info, term_width, "[%s:%u %s()]", basename((char*)file), line_no, function);
        debug_offset = (term_width-1) - debug_len;

        if (debug_offset > 0) {
            memcpy((char*)(out_line + debug_offset), debug_info, debug_len);
        }

        SFREE(debug_info);
    }


    // Cap the string
    out_line[term_width] = '\0';

    out_bytes           += LogLine(level, "%s", out_line);

    // printf("%s\n", out_line);

    // Clean up.
    va_end(argp);
    SFREE(in_line);
    SFREE(out_line);

    return out_bytes;
}

