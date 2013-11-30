//
//  output.c
//  hfsinspect
//
//  Created by Adam Knight on 11/10/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "stringtools.h"
#include "output.h"
#include <stdarg.h>         //va_list, etc.
#include <sys/param.h>      //MIN/MAX
#include <uuid/uuid.h>      //uuid_t, etc.

char _indent_string[50] = "";
unsigned indent_step = 2;

static inline const unsigned long indent_level() {
    return strnlen(_indent_string, 50);
}

static inline const void set_indent_level(unsigned long new_level) {
    if (new_level > 49) new_level = 0;
    _indent_string[new_level] = '\0';
    while (new_level) _indent_string[--new_level] = ' ';
}


#pragma mark Print Functions

int BeginSection(const char* format, ...)
{
    va_list argp;
    va_start(argp, format);
    
    unsigned long indent = indent_level();
    
    Print("");
    char str[255];
    vsprintf(str, format, argp);
    int bytes = Print("# %s", str);
    
    set_indent_level(indent + indent_step);
    
    va_end(argp);
    return bytes;
}

void EndSection(void)
{
    unsigned long indent = indent_level();
    if (indent > 0) set_indent_level(indent - MIN(indent, indent_step));
}

int Print(const char* format, ...)
{
    va_list argp;
    va_start(argp, format);
    
    char str[255]; vsprintf(str, format, argp);
    int bytes = print("%s%s", _indent_string, str);
    
    va_end(argp);
    return bytes;
}

int PrintAttribute(const char* label, const char* format, ...)
{
    va_list argp;
    va_start(argp, format);
    
    int bytes = 0;
    char str[255]; vsprintf(str, format, argp);
    char spc[2] = {' ', '\0'};
    
    if (label == NULL)
        bytes = Print("%-23s. %s", "", str);
    else if ( memcmp(label, spc, 2) == 0 )
        bytes = Print("%-23s  %s", "", str);
    else
        bytes = Print("%-23s= %s", label, str);
    
    va_end(argp);
    return bytes;
}

#pragma mark Line Print Functions

void _PrintDataLength(const char *label, uint64_t size)
{
    char sizeLabel[50];
    char metricLabel[50];
    if (size > 1024*1024) {
        (void)format_size(sizeLabel, size, false, 50);
        (void)format_size(metricLabel, size, true, 50);
        PrintAttribute(label, "%s/%s (%lu bytes)", sizeLabel, metricLabel, size);
    } else if (size > 1024) {
        (void)format_size(sizeLabel, size, false, 50);
        PrintAttribute(label, "%s (%lu bytes)", sizeLabel, size);
    } else {
        PrintAttribute(label, "%lu bytes", size);
    }
}

void PrintAttributeDump(const char* label, const void* map, size_t nbytes, char base)
{
    PrintAttribute(label, "");
    VisualizeData(map, nbytes);
}

void _PrintRawAttribute(const char* label, const void* map, size_t nbytes, char base)
{
    unsigned segmentLength = 32;
    
    ssize_t msize = format_dump(NULL, map, base, nbytes, 0);
    if (msize < 0) { perror("format_dump"); return; }
    msize++; // NULL terminator
    char* str;
    INIT_BUFFER(str, msize);
    
    size_t len = format_dump(str, map, base, nbytes, msize);
    
    for (int i = 0; i < len; i += segmentLength) {
        char segment[segmentLength]; memset(segment, '\0', segmentLength);
        
        strlcpy(segment, &str[i], MIN(segmentLength, len - i));
        
        PrintAttribute(label, "%s%s", (base==16?"0x":""), segment);
        
        if (i == 0) label = "";
    }
    
    FREE_BUFFER(str);
}

void VisualizeData(const void* data, size_t length)
{
    memdump(stdout, data, length, 16, 4, 8, DUMP_ADDRESS | DUMP_ENCODED | DUMP_OFFSET | DUMP_ASCII | DUMP_PADDING);
}


#pragma mark - Formatters

int format_dump(char* out, const char* value, unsigned base, size_t nbytes, size_t length)
{
    return memstr(out, base, value, nbytes, length);
}

int format_size(char* out, size_t value, bool metric, size_t length)
{
    float divisor = 1024.0;
    if (metric) divisor = 1000.0;
    
    char* sizeNames[] = { "bytes", "KiB", "MiB", "GiB", "TiB", "EiB", "PiB", "ZiB", "YiB" };
    char* metricNames[] = { "bytes", "KB", "MB", "GB", "TB", "EB", "PB", "ZB", "YB" };
    
    long double displaySize = value;
    int count = 0;
    while (count < 9) {
        if (displaySize < divisor) break;
        displaySize /= divisor;
        count++;
    }
    char* sizeLabel;
    
    if (metric) sizeLabel = metricNames[count]; else sizeLabel = sizeNames[count];
    
    snprintf(out, length, "%0.2Lf %s", displaySize, sizeLabel);
    
    return strlen(out);
}

int format_blocks(char* out, size_t blocks, size_t block_size, size_t length)
{
    size_t displaySize = (blocks * block_size);
    char sizeLabel[50];
    (void)format_size(sizeLabel, displaySize, false, 50);
    return snprintf(out, length, "%s (%zu blocks)", sizeLabel, blocks);
}

int format_time(char* out, time_t gmt_time, size_t length)
{
    struct tm *t = gmtime(&gmt_time);
    return (int)strftime(out, length, "%c %Z\0", t);
}

int format_uint_oct(char* out, uint64_t value, uint8_t padding, size_t length)
{
    char format[100];
    snprintf(format, 100, "0%%%ullo", padding);
    return snprintf(out, length, format, value);
}

int format_uint_dec(char* out, uint64_t value, uint8_t padding, size_t length)
{
    char format[100];
    snprintf(format, 100, "%%%ullu", padding);
    return snprintf(out, length, format, value);
}

int format_uint_hex(char* out, uint64_t value, uint8_t padding, size_t length)
{
    char format[100];
    snprintf(format, 100, "0x%%%ullx", padding);
    return snprintf(out, length, format, value);
}

int format_uuid(char* out, const unsigned char value[16])
{
    uuid_unparse(value, out);
    return strlen(out);
}

int format_uint_chars(char* out, const char* value, size_t nbytes, size_t length)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    while (nbytes--) *out++ = value[nbytes];
    *out++ = '\0';
#else
    memcpy(out, value, nbytes);
    out[nbytes] = '\0';
#endif
    return strlen(out);
}
