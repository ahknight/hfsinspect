//
//  output.h
//  volumes
//
//  Created by Adam Knight on 11/10/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef volumes_output_h
#define volumes_output_h

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#define MEMBER_LABEL(s, m) #m, s->m

struct out_ctx {
    unsigned indent_level;
    unsigned indent_step;
    char     indent_string[32];
    char*    prefix;
    bool     decimal_sizes;
    uint8_t  _reserved[7];
};
typedef struct out_ctx out_ctx;

out_ctx OCMake          (bool decimal_sizes, unsigned indent_step, char* prefix);
void    OCSetIndentLevel(out_ctx* ctx, unsigned new_level);

void PrintAttributeDump (out_ctx* ctx, const char* label, const void* map, size_t nbytes, char base);
void _PrintRawAttribute (out_ctx* ctx, const char* label, const void* map, size_t size, char base);
void _PrintDataLength   (out_ctx* ctx, const char* label, uint64_t size);
void VisualizeData      (const void* data, size_t length);

#define PrintRawAttribute(ctx, record, value, base) _PrintRawAttribute(ctx, #value, &(record->value), sizeof(record->value), base)
#define PrintDataLength(ctx, record, value)         _PrintDataLength(ctx, #value, (uint64_t)record->value)
#define PrintUIChar(ctx, record, value)             _PrintUIChar(ctx, #value, (char*)&(record->value), sizeof(record->value))

#define PrintUI(ctx, record, value)                 PrintAttribute(ctx, #value, "%llu", (uint64_t)record->value)
#define PrintUIOct(ctx, record, value)              PrintAttribute(ctx, #value, "0%06o (%u)", record->value, record->value)
#define PrintUIHex(ctx, record, value)              PrintAttribute(ctx, #value, "%#x (%u)", record->value, record->value)

#define PrintIntFlag(ctx, label, name, value)       PrintAttribute(ctx, label, "%s (%llu)", name, (uint64_t)value)
#define PrintOctFlag(ctx, label, name, value)       PrintAttribute(ctx, label, "0%06o (%s)", value, name)
#define PrintHexFlag(ctx, label, name, value)       PrintAttribute(ctx, label, "%s (%#x)", name, value)

#define PrintUIFlagIfSet(ctx, source, flag)         { if (((uint64_t)(source)) & (((uint64_t)1) << ((uint64_t)(flag)))) PrintIntFlag(ctx, #source, #flag, flag); }
#define PrintUIFlagIfMatch(ctx, source, mask)       { if ((source) & mask) PrintIntFlag(ctx, #source, #mask, mask); }
#define PrintUIOctFlagIfMatch(ctx, source, mask)    { if ((source) & mask) PrintOctFlag(ctx, #source, #mask, mask); }

#define PrintConstIfEqual(ctx, source, c)           { if ((source) == c)   PrintIntFlag(ctx, #source, #c, c); }
#define PrintConstOctIfEqual(ctx, source, c)        { if ((source) == c)   PrintOctFlag(ctx, #source, #c, c); }
#define PrintConstHexIfEqual(ctx, source, c)        { if ((source) == c)   PrintHexFlag(ctx, #source, #c, c); }

int  BeginSection   (out_ctx* ctx, const char* format, ...);
void EndSection     (out_ctx* ctx);

int Print           (out_ctx* ctx, const char* format, ...);
int PrintAttribute  (out_ctx* ctx, const char* label, const char* format, ...);
int _PrintUIChar    (out_ctx* ctx, const char* label, const char* i, size_t nbytes);

int format_dump     (out_ctx* ctx, char* out, const char* value, unsigned base, size_t nbytes, size_t length);
int format_size     (out_ctx* ctx, char* out, size_t value, size_t length);
int format_blocks   (out_ctx* ctx, char* out, size_t blocks, size_t block_size, size_t length);
int format_time     (out_ctx* ctx, char* out, time_t gmt_time, size_t length);

int format_uint_oct (char* out, uint64_t value, uint8_t padding, size_t length);
int format_uint_dec (char* out, uint64_t value, uint8_t padding, size_t length);
int format_uint_hex (char* out, uint64_t value, uint8_t padding, size_t length);
int format_uuid     (char* out, const unsigned char value[16]);
int format_uint_chars(char* out, const char* value, size_t nbytes, size_t length);

#endif
