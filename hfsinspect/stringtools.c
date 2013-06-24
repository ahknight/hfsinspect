//
//  stringtools.c
//  hfsinspect
//
//  Created by Adam Knight on 6/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "stringtools.h"

#include <stdio.h>

// repeat character
char* repchar(char c, int count)
{
    char* str = malloc( (sizeof(c) * count) + 1);
    memset(str, c, count);
    str[count] = '\0';
    return str;
}

// repeat wide character
wchar_t* repwchar(wchar_t c, int count)
{
    wchar_t* str = malloc(sizeof(c) * count);
    memset(str, c, count);
    str[count] = '\0';
    return str;
}

// Generate a human-readable string representing file size
char* sizeString(size_t size, bool metric)
{
    float divisor = 1024.0;
    if (metric) divisor = 1000.0;

    char* sizeNames[] = { "bytes", "KiB", "MiB", "GiB", "TiB", "EiB", "PiB", "ZiB", "YiB" };
    char* metricNames[] = { "bytes", "KB", "MB", "GB", "TB", "EB", "PB", "ZB", "YB" };

    long double displaySize = size;
    int count = 0;
    while (count < 9) {
        if (displaySize < divisor) break;
        displaySize /= divisor;
        count++;
    }
    char* sizeLabel;

    if (metric) sizeLabel = metricNames[count]; else sizeLabel = sizeNames[count];
    
    char* label = NULL;
    
    asprintf(&label, "%0.2Lf %s", displaySize, sizeLabel);
    
    return label;
}

// Generate a string representation of a buffer with adjustable number base (2-36).
char* memstr(const char* buf, size_t length, u_int8_t base)
{
    // Base 1 would be interesting (ie. infinite). Past base 36 and we need to start picking printable character sets.
    if (base < 2 || base > 36) return NULL;
    
    // Determine how many characters will be needed in the output for each byte of input (256 bits, 8 bits to a byte).
    u_int8_t chars_per_byte = (256 / base / 8);
    
    // Patches bases over 32.
    if (chars_per_byte < 1) chars_per_byte = 1;
    
    // Size of the result string.
    size_t rlength = (length * chars_per_byte);
    char* result;
    INIT_STRING(result, rlength + 1);
    
    // Init the string's bytes to the character zero so that positions we don't write to have something printable.
    memset(result, '0', malloc_size(result));
    
    // We build the result string from the tail, so here's the index in that string, starting at the end.
    u_int8_t ridx = rlength - 1;
    for (size_t byte = 0; byte < length; byte++) {
        unsigned char c = buf[byte];    // Grab the current char from the input.
        while (c != 0 && ridx >= 0) {   // Iterate until we're out of input or output.
            u_int8_t idx = c % base;
            result[ridx] = (idx < 10 ? '0' + idx : 'A' + (idx-10)); // GO ASCII! 0-9 then A-Z (up to base 36, see above).
            c /= base;                  // Shave off the processed portion.
            ridx--;                     // Move left in the result string.
        }
    }
    
    // Cap the string.
    result[rlength] = '\0';
    
    // Send it back.
    return result;
}

// Print a classic memory dump with configurable base and grouping.
void memdump(const char* data, size_t length, u_int8_t base, u_int8_t gsize, u_int8_t gcount)
{
    int group_size = gsize;
    int group_count = gcount;
    
    int line_width = group_size * group_count;
    unsigned int offset = 0;
    while (length > offset) {
        const char* line = &data[offset];
        size_t lineMax = MIN((length - offset), line_width);
        printf("  %p %06u |", &line[0], offset);
        for (int c = 0; c < lineMax; c++) {
            if ( (c % group_size) == 0 ) printf(" ");
            char* str = memstr(&line[c], 1, base);
            printf("%s ", str); FREE_BUFFER(str);
        }
        printf("|");
        for (int c = 0; c < lineMax; c++) {
            if ( (c % group_size) == 0 ) printf(" ");
            char chr = line[c] & 0xFF;
            if (chr < 32) // ASCII unprintable
                chr = '.';
            printf("%c", chr);
        }
        printf(" |\n");
        offset += line_width;
    }
}

