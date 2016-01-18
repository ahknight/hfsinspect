//
//  output.c
//  volumes
//
//  Created by Adam Knight on 11/10/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <time.h>           // gmtime

#include "output.h"

#include "logging/logging.h"    // console printing routines
#include "memdmp/memdmp.h"
#include "memdmp/output.h"

out_ctx OCMake(bool decimal_sizes, unsigned indent_step, char* prefix)
{
    out_ctx ctx = {
        .decimal_sizes = decimal_sizes,
        .indent_level  = 0,
        .indent_step   = 2,
        .prefix        = prefix,
        .indent_string = ""
    };
    return ctx;
}

void OCSetIndentLevel(out_ctx* ctx, unsigned new_level)
{
    ctx->indent_level = new_level;

    if (new_level > (sizeof(ctx->indent_string) - 1))
        new_level = 0;

    memset(ctx->indent_string, ' ', sizeof(ctx->indent_string));
    ctx->indent_string[new_level] = '\0';
}

#pragma mark - Print Functions

int BeginSection(out_ctx* ctx, const char* format, ...)
{
    va_list argp;
    va_start(argp, format);

    Print(ctx, "");
    char    str[255];
    vsprintf(str, format, argp);
    int     bytes = Print(ctx, "# %s", str);

    OCSetIndentLevel(ctx, ctx->indent_level + ctx->indent_step);

    va_end(argp);
    return bytes;
}

void EndSection(out_ctx* ctx)
{
    if (ctx->indent_level > 0)
        OCSetIndentLevel(ctx, ctx->indent_level - MIN(ctx->indent_level, ctx->indent_step));
}

int Print(out_ctx* ctx, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);

    fputs(ctx->indent_string, stdout);
    int     bytes = vfprintf(stdout, format, ap);
    fputs("\n", stdout);

    va_end(ap);
    return bytes;
}

int PrintAttribute(out_ctx* ctx, const char* label, const char* format, ...)
{
    va_list argp;
    va_start(argp, format);

    int     bytes  = 0;
    char    str[255];
    char    spc[2] = {' ', '\0'};

    vsprintf(str, format, argp);

    if (label == NULL)
        bytes = Print(ctx, "%-23s. %s", "", str);
    else if ( memcmp(label, spc, 2) == 0 )
        bytes = Print(ctx, "%-23s  %s", "", str);
    else
        bytes = Print(ctx, "%-23s= %s", label, str);

    va_end(argp);
    return bytes;
}

#pragma mark Line Print Functions

void _PrintDataLength(out_ctx* ctx, const char* label, uint64_t size)
{
    assert(label != NULL);

    char decimalLabel[50];

    (void)format_size(ctx, decimalLabel, size, 50);

    if (size > 1024) {
        PrintAttribute(ctx, label, "%s (%lu bytes)", decimalLabel, size);
    } else {
        PrintAttribute(ctx, label, "%s", decimalLabel);
    }
}

void PrintAttributeDump(out_ctx* ctx, const char* label, const void* map, size_t nbytes, char base)
{
    assert(label != NULL);
    assert(map != NULL);
    assert(nbytes > 0);
    assert(base >= 2 && base <= 36);

    PrintAttribute(ctx, label, "");
    VisualizeData(map, nbytes);
}

void _PrintRawAttribute(out_ctx* ctx, const char* label, const void* map, size_t nbytes, char base)
{
    assert(label != NULL);
    assert(map != NULL);
    assert(nbytes > 0);
    assert(base >= 2 && base <= 36);

#define segmentLength 32
    char*   str   = NULL;
    ssize_t len   = 0;
    ssize_t msize = 0;

    msize = format_dump(ctx, NULL, map, base, nbytes, 0);
    if (msize < 0) { perror("format_dump"); return; }
    msize++; // NULL terminator
    SALLOC(str, msize);

    if ( (len = format_dump(ctx, str, map, base, nbytes, msize)) < 0 ) {
        warning("format_dump failed!");
        return;
    }

    for (unsigned i = 0; i < len; i += segmentLength) {
        char segment[segmentLength]; memset(segment, '\0', segmentLength);

        (void)strlcpy(segment, &str[i], MIN(segmentLength, len - i));

        PrintAttribute(ctx, label, "%s%s", (base==16 ? "0x" : ""), segment);

        if (i == 0) label = "";
    }

    SFREE(str);
}

int _PrintUIChar(out_ctx* ctx, const char* label, const char* i, size_t nbytes)
{
    char str[50] = {0};
    char hex[50] = {0};

    (void)format_uint_chars(str, i, nbytes, 50);
    (void)format_dump(ctx, hex, i, 16, nbytes, 50);

    return PrintAttribute(ctx, label, "0x%s (%s)", hex, str);
}

void VisualizeData(const void* data, size_t length)
{
    // Init the last line to something unlikely so a zero line is shown.
    memdmp_state state = { .prev = "5432" };

    memdmp(stdout, data, length, NULL, &state);
}

#pragma mark - Formatters

int format_dump(out_ctx* ctx, char* out, const char* value, unsigned base, size_t nbytes, size_t length)
{
    assert(value != NULL);
    assert(base >= 2 && base <= 36);
    assert(nbytes > 0);
    if (out != NULL) assert(length > 0);

    return memstr(out, base, value, nbytes, length);
}

int format_size(out_ctx* ctx, char* out, size_t value, size_t length)
{
    char*       binaryNames[]  = { "bytes", "KiB", "MiB", "GiB", "TiB", "EiB", "PiB", "ZiB", "YiB" };
    char*       decimalNames[] = { "bytes", "KB", "MB", "GB", "TB", "EB", "PB", "ZB", "YB" };
    float       divisor        = 1024.0;
    bool        decimal        = ctx->decimal_sizes;
    long double displaySize    = value;
    int         count          = 0;
    char*       sizeLabel;

    assert(ctx != NULL);
    assert(out != NULL);
    assert(length > 0);

    if (decimal) divisor = 1000.0;

    while (count < 9) {
        if (displaySize < divisor) break;
        displaySize /= divisor;
        count++;
    }

    if (decimal) sizeLabel = decimalNames[count]; else sizeLabel = binaryNames[count];

    snprintf(out, length, "%0.2Lf %s", displaySize, sizeLabel);

    return strlen(out);
}

int format_blocks(out_ctx* ctx, char* out, size_t blocks, size_t block_size, size_t length)
{
    size_t displaySize = (blocks * block_size);
    char   sizeLabel[50];
    (void)format_size(ctx, sizeLabel, displaySize, 50);
    return snprintf(out, length, "%s (%zu blocks)", sizeLabel, blocks);
}

int format_time(out_ctx* ctx, char* out, time_t gmt_time, size_t length)
{
    struct tm* t = gmtime(&gmt_time);
#if defined(__APPLE__)
    return (int)strftime(out, length, "%c %Z", t);
#else
    return (int)strftime(out, length, "%c", t);
#endif
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
    *out++      = '\0';
#else
    memcpy(out, value, nbytes);
    out[nbytes] = '\0';
#endif
    return strlen(out);
}

