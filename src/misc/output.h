//
//  output.h
//  hfsinspect
//
//  Created by Adam Knight on 11/10/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_output_h
#define hfsinspect_output_h

void output_set_uses_decimal_blocks(bool value);

#define MEMBER_LABEL(s, m) #m, s->m

void PrintAttributeDump (const char* label, const void* map, size_t nbytes, char base);
void _PrintRawAttribute (const char* label, const void* map, size_t size, char base);
void _PrintDataLength   (const char* label, uint64_t size);
void VisualizeData      (const void* data, size_t length);


#define PrintUI(record, value)                  PrintAttribute(#value, "%llu", (uint64_t)record->value)
#define PrintUIOct(record, value)               PrintAttribute(#value, "0%06o (%u)", record->value, record->value)
#define PrintUIHex(record, value)               PrintAttribute(#value, "%#x (%u)", record->value, record->value)

#define PrintRawAttribute(record, value, base)  _PrintRawAttribute(#value, &(record->value), sizeof(record->value), base)

#define PrintDataLength(record, value)          _PrintDataLength(#value, (uint64_t)record->value)

#define PrintOctFlag(label, name, value)        PrintAttribute(label, "0%06o (%s)", value, name)
#define PrintHexFlag(label, name, value)        PrintAttribute(label, "%s (%#x)", name, value)
#define PrintIntFlag(label, name, value)        PrintAttribute(label, "%s (%llu)", name, (uint64_t)value)

#define PrintUIFlagIfSet(source, flag)          { if (((uint64_t)(source)) & (((uint64_t)1) << ((uint64_t)(flag)))) PrintIntFlag(#source, #flag, flag); }
#define PrintUIFlagIfMatch(source, mask)        { if ((source) & mask) PrintIntFlag(#source, #mask, mask); }
#define PrintUIOctFlagIfMatch(source, mask)     { if ((source) & mask) PrintOctFlag(#source, #mask, mask); }
#define PrintUIHexFlagIfMatch(source, mask)     { if ((source) & mask) PrintHexFlag(#source, #mask, mask); }

#define PrintConstIfEqual(source, c)            { if ((source) == c)   PrintIntFlag(#source, #c, c); }
#define PrintConstOctIfEqual(source, c)         { if ((source) == c)   PrintOctFlag(#source, #c, c); }
#define PrintConstHexIfEqual(source, c)         { if ((source) == c)   PrintHexFlag(#source, #c, c); }



int BeginSection    (const char* format, ...);
void EndSection     (void);

int Print           (const char* format, ...);
int PrintAttribute  (const char* label, const char* format, ...);



int format_dump     (char* out, const char* value, unsigned base, size_t nbytes, size_t length);
int format_size     (char* out, size_t value, size_t length);
int format_size_d   (char* out, size_t value, size_t length, bool decimal);
int format_blocks   (char* out, size_t blocks, size_t block_size, size_t length);
int format_time     (char* out, time_t gmt_time, size_t length);

int format_uint_oct (char* out, uint64_t value, uint8_t padding, size_t length);
int format_uint_dec (char* out, uint64_t value, uint8_t padding, size_t length);
int format_uint_hex (char* out, uint64_t value, uint8_t padding, size_t length);
int format_uuid     (char* out, const unsigned char value[16]);
int format_uint_chars(char* out, const char* value, size_t nbytes, size_t length);

#endif
