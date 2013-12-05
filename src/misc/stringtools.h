//
//  stringtools.h
//  hfsinspect
//
//  Created by Adam Knight on 6/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_stringtools_h
#define hfsinspect_stringtools_h

#include <stdio.h>  //FILE*
#include <wchar.h>  //wchar_t
#include "hfs/hfs_types.h" // hfs_wc_str

enum {
    DUMP_ADDRESS    = 0x0001,
    DUMP_OFFSET     = 0x0002,
    DUMP_ENCODED    = 0x0004,
    DUMP_ASCII      = 0x0008,
    DUMP_FULL       = 0x00FF,
    
    DUMP_PADDING    = 0x0100,
};

// Generate a string representation of a buffer with adjustable number base (2-36).
ssize_t memstr(char* restrict out, uint8_t base, const char* input, size_t nbytes, size_t length);

// Print a classic memory dump with configurable base and grouping.
void memdump(FILE* file, const char* data, size_t length, uint8_t base, uint8_t width, uint8_t groups, unsigned mode);

#pragma mark UTF conversions

_NONNULL    int             hfsuctowcs                      (hfs_wc_str output, const HFSUniStr255* input);
_NONNULL    HFSUniStr255    wcstohfsuc                      (const wchar_t* input);
_NONNULL    HFSUniStr255    strtohfsuc                      (const char* input);

// http://unicode.org/faq/utf_bom.html#utf16-3

typedef uint16_t    UTF16;
typedef uint32_t    UTF32;

typedef UTF32   UChar32;

typedef struct UChar16 {
    UTF16   hi;
    UTF16   lo;
} UChar16;

UChar32 uc16touc32( UChar16 u16 );
UChar16 uc32touc16( UChar32 codepoint );

#endif
