//
//  hfs/unicode.c
//  hfsinspect
//
//  Created by Adam Knight on 2015-02-01.
//  Copyright (c) 2015 Adam Knight. All rights reserved.
//

#include <string.h>

#include "hfs/unicode.h"
#include "hfs/catalog.h"        // HFSPlusMetadataFolder
#include "logging/logging.h"    // console printing routines

int hfsuctowcs(hfs_wc_str output, const HFSUniStr255* input)
{
    trace("output (%p), input (%p)", output, input);

    // Get the length of the input
    unsigned len = MIN(input->length, 255);

    // Copy the u16 to the wchar array
    for(unsigned i = 0; i < len; i++)
        output[i] = input->unicode[i];

    // Terminate the output at the length
    output[len] = L'\0';

    // Replace the catalog version with a printable version.
    if ( wcscmp(output, HFSPlusMetadataFolder) == 0 )
        mbstowcs(output, HFSPLUSMETADATAFOLDER, len);

    return len;
}

// FIXME: Eats higher-order Unicode chars. Could be fun.
HFSUniStr255 wcstohfsuc(const wchar_t* input)
{
    trace("input '%ls'", input);

    // Allocate the return value
    HFSUniStr255 output = {0};

    // Get the length of the input
    size_t       len    = MIN(wcslen(input), 255);

    // Iterate over the input
    for (unsigned i = 0; i < len; i++) {
        // Copy the input to the output
        output.unicode[i] = input[i];
    }

    // Terminate the output at the length
    output.length = len;

    return output;
}

HFSUniStr255 strtohfsuc(const char* input)
{
    HFSUniStr255 output          = {0};
    wchar_t*     wide            = NULL;
    size_t       char_count      = strlen(input);
    size_t       wide_char_count = mbstowcs(wide, input, 255);

    trace("input '%s'", input)

    SALLOC(wide, 256 * sizeof(wchar_t));

    if (wide_char_count > 0) output = wcstohfsuc(wide);

    if (char_count != wide_char_count) {
        error("Conversion error: mbstowcs returned a string of a different length than the input: %zd in; %zd out", char_count, wide_char_count);
    }
    SFREE(wide);

    return output;
}

#ifndef __GNUC__
const UTF16 HI_SURROGATE_START = 0xD800;
const UTF16 LO_SURROGATE_START = 0xDC00;

const UTF32 LEAD_OFFSET        = HI_SURROGATE_START - (0x10000 >> 10);
const UTF32 SURROGATE_OFFSET   = 0x10000 - (HI_SURROGATE_START << 10) - LO_SURROGATE_START;

UChar32 uc16touc32( UChar16 u16 )
{
    return ( (u16.hi << 10) + u16.lo + SURROGATE_OFFSET );
}

UChar16 uc32touc16( UChar32 codepoint )
{
    UChar16 u16;
    u16.hi = LEAD_OFFSET + (codepoint >> 10);
    u16.lo = 0xDC00 + (codepoint & 0x3FF);
    return u16;
}

#endif
