//
//  stringtools.c
//  hfsinspect
//
//  Created by Adam Knight on 6/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "stringtools.h"
#include "hfs_macos_defs.h"

#include <errno.h>      //errno
#include <sys/param.h>  //MIN/MAX
#include <hfs/hfs_format.h>
#include <math.h>

/**
 Get a string representation of a block of memory in any reasonable number base.
 @param out A buffer with enough space to hold the output. Pass NULL to get the size to allocate.
 @param base The base to render the text as (2-36).
 @param buf The buffer to render.
 @param length The size of the buffer.
 @return The number of characters written to *out (or that would be written), or -1 on error.
 */
ssize_t memstr(char* restrict out, uint8_t base, const char* input, size_t nbytes, size_t length)
{
    /* Verify parameters */
    
    // Zero length buffer == zero length result.
    if (nbytes == 0) return 0;
    
    // No source buffer?
    if (input == NULL) { errno = EINVAL; return -1; }
    
    // Base 1 would be interesting (ie. infinite). Past base 36 and we need to start picking printable character sets.
    if (base < 2 || base > 36) { errno = EINVAL; return -1; }
    
    // Determine how many characters will be needed in the output for each byte of input (256 bits, 8 bits to a byte).
    uint8_t chars_per_byte = (log10(256)/log10(base));
    
    // Patches bases over 32.
    if (chars_per_byte < 1) chars_per_byte = 1;
    
    // Determine the size of the result string.
    size_t rlength = (nbytes * chars_per_byte);
    
    // Return the size if the output buffer is empty.
    if (out == NULL) return rlength;
    
    // Ensure we have been granted enough space to do this.
    if (rlength > length) { errno = ENOMEM; return -1; }
    
    
    /* Process the string */
    
    // Init the string's bytes to the character zero so that positions we don't write to have something printable.
    // THIS IS INTENTIONALLY NOT '\0'!!
    memset(out, '0', rlength);
    
    // We build the result string from the tail, so here's the index in that string, starting at the end.
    uint8_t ridx = rlength - 1;
    for (size_t byte = 0; byte < nbytes; byte++) {
        uint8_t chr = input[byte];      // Grab the current char from the input.
        while (chr != 0 && ridx >= 0) { // Iterate until we're out of input or output.
            uint8_t idx = chr % base;
            out[ridx] = (idx < 10 ? '0' + idx : 'a' + (idx-10)); // GO ASCII! 0-9 then a-z (up to base 36, see above).
            chr /= base;                // Shave off the processed portion.
            ridx--;                     // Move left in the result string.
        }
    }
    
    // Cap the string.
    out[rlength] = '\0';
    
    return rlength;
}


/**
 Print a block of memory to stdout in a stylized fashion, ASCII to the right.
 | 00000000 00000000 00000000 00000000 | ................ |
 (Above: four groups with width four)
 
 @param file File pointer to print to.
 @param data A buffer of data to print.
 @param length The number of bytes from the data pointer to print.
 @param base The number base to use when printing (middle column).
 @param width The number of input bytes to group together.
 @param groups The number of groups per line.
 @param mode An ORed value denoting what to include in each line.
 */

void memdump(FILE* file, const char* data, size_t length, uint8_t base, uint8_t width, uint8_t groups, unsigned mode)
{
    // Length of a line
    int line_width = width * groups;
    
    off_t offset = 0;
    while (length > offset) {
        // Starting position for the line
        const char* line = &data[offset];
        
        if ( (offset - line_width) > 0 && (offset + line_width) < length) {
            const char* prevLine = &data[offset - line_width];
            const char* nextLine = &data[offset + line_width];
            
            // Do we match the previous line?
            if (memcmp(prevLine, line, line_width) == 0) {
                // How about the next one?
                if (memcmp(nextLine, line, line_width) == 0) {
                    // Great! Are we the first one to do this?
                    if (memcmp((prevLine - line_width), line, line_width) != 0) {
                        if (mode & DUMP_ADDRESS) fprintf(file, "%12s", "");
                        if (mode & DUMP_OFFSET)  fprintf(file, "%8s", "");
                        fprintf(file, "  ...\n");
                    }
                    goto NEXT;
                }
            }
        }
        
        // Length of this line (considering the last line may not be a full line)
        size_t lineMax = MIN((length - offset), line_width);
        
        // Line header/prefix
        if (mode & DUMP_ADDRESS) fprintf(file, "%12p", &line[0]);
        if (mode & DUMP_OFFSET)  fprintf(file, "%8lld", offset);
        
        // Print numeric representation
        if (mode & DUMP_ENCODED) {
            fprintf(file, " ");
            for (size_t c = 0; c < lineMax; c++) {
                size_t size = memstr(NULL, base, &line[c], 1, 0);
                if (size) {
                    if ((c % width) == 0) fprintf(file, " ");
                    char group[100] = "";
                    if (line[c] == 0)
                        memset(group, '-', size);
                    else
                        memstr(group, base, &line[c], 1, size);
                    
                    fprintf(file, "%s%s", group, ((mode & DUMP_PADDING) ? " " : ""));
                } else {
                    break; // for - clearly we've run out of input.
                }
            }
        }
        
        // Print ASCII representation
        if (mode & DUMP_ASCII) {
            fprintf(file, " |");
            for (int c = 0; c < lineMax; c++) {
                uint8_t chr = line[c] & 0xFF;
                if (chr == 0)
                    chr = ' ';
                else if (chr > 127) // ASCII unprintable
                    chr = '?';
                else if (chr < 32) // ASCII control characters
                    chr = '.';
                fprintf(file, "%c", chr);
            }
            fprintf(file, "|");
        }
        
        fprintf(file, "\n");
        
    NEXT:
        offset += line_width;
    }
}


const UTF16 HI_SURROGATE_START = 0xD800;
const UTF16 LO_SURROGATE_START = 0xDC00;

const UTF32 LEAD_OFFSET = HI_SURROGATE_START - (0x10000 >> 10);
const UTF32 SURROGATE_OFFSET = 0x10000 - (HI_SURROGATE_START << 10) - LO_SURROGATE_START;

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

