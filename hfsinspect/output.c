//
//  output.c
//  hfsinspect
//
//  Created by Adam Knight on 11/10/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "stringtools.h"
#include "output.h"
#include <stdarg.h>
#include <sys/param.h>      //MIN/MAX

#pragma mark Line Print Functions

void PrintString(const char* label, const char* value_format, ...)
{
    va_list argp;
    va_start(argp, value_format);
    char* str;
    vasprintf(&str, value_format, argp);
    printf("  %-23s  %s\n", label, str);
    FREE_BUFFER(str);
    va_end(argp);
}

void PrintHeaderString(const char* value_format, ...)
{
    va_list argp;
    va_start(argp, value_format);
    char* str;
    vasprintf(&str, value_format, argp);
    printf("\n# %s\n", str);
    FREE_BUFFER(str);
    va_end(argp);
}

void PrintAttributeString(const char* label, const char* value_format, ...)
{
    va_list argp;
    va_start(argp, value_format);
    char* str;
    vasprintf(&str, value_format, argp);
    printf("  %-23s= %s\n", label, str);
    FREE_BUFFER(str);
    va_end(argp);
}

void PrintSubattributeString(const char* str, ...)
{
    va_list argp;
    va_start(argp, str);
    char* s;
    vasprintf(&s, str, argp);
    printf("  %-23s. %s\n", "", s);
    FREE_BUFFER(s);
    va_end(argp);
}

void _PrintDataLength(const char *label, uint64_t size)
{
    size_t displaySize = size;
    char sizeLabel[50];
    char metricLabel[50];
    if (size > 1024*1024) {
        (void)format_size(sizeLabel, displaySize, false, 50);
        (void)format_size(metricLabel, displaySize, true, 50);
        PrintAttributeString(label, "%s/%s (%lu bytes)", sizeLabel, metricLabel, size);
    } else if (size > 1024) {
        (void)format_size(sizeLabel, displaySize, false, 50);
        PrintAttributeString(label, "%s (%lu bytes)", sizeLabel, size);
    } else {
        PrintAttributeString(label, "%lu bytes", size);
    }
}

void _PrintRawAttribute(const char* label, const void* map, size_t size, char base)
{
    unsigned segmentLength = 32;
    ssize_t msize = memstr(NULL, base, map, size);
    if (msize < 0) {
        perror("memstr");
        return;
    }
    
    char* str;
    INIT_STRING(str, msize);
    
    size_t len = memstr(str, base, map, size);
    
    for (int i = 0; i < len; i += segmentLength) {
        char segment[segmentLength]; memset(segment, '\0', segmentLength);
        
        strlcpy(segment, &str[i], MIN(segmentLength, len - i));
        
        PrintAttributeString(label, "%s%s", (base==16?"0x":""), segment);
        
        if (i == 0) label = "";
    }
    
    FREE_BUFFER(str);
}

void VisualizeData(const char* data, size_t length)
{
    memdump(stdout, data, length, 16, 4, 8, DUMP_ENCODED | DUMP_OFFSET | DUMP_ASCII | DUMP_PADDING);
}
