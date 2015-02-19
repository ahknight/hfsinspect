//
//  hfs/unicode.h
//  hfsinspect
//
//  Created by Adam Knight on 2015-02-01.
//  Copyright (c) 2015 Adam Knight. All rights reserved.
//

#ifndef hfs_unicode_h
#define hfs_unicode_h

#include <stdint.h>         // uint*
#include <wchar.h>          // wchar_t
#include "hfs/types.h"      // hfs_wc_str

#pragma mark UTF conversions

int          hfsuctowcs (hfs_wc_str output, const HFSUniStr255* input) __attribute__((nonnull));
HFSUniStr255 wcstohfsuc (const wchar_t* input) __attribute__((nonnull));
HFSUniStr255 strtohfsuc (const char* input) __attribute__((nonnull));

// http://unicode.org/faq/utf_bom.html#utf16-3

typedef uint16_t UTF16;
typedef uint32_t UTF32;

typedef struct UChar16 {
    UTF16 hi;
    UTF16 lo;
} UChar16;

typedef UTF32 UChar32;

UChar32 UChar16toUChar32( UChar16 u16 );
UChar16 UChar32toUChar16( UChar32 codepoint );
UTF16   UChar16toUTF16(UChar16 u16);

#endif
