//
//  stringtools.c
//  hfsinspect
//
//  Created by Adam Knight on 6/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "stringtools.h"

//#include <string.h>             // memcpy, strXXX, etc.
//#include <sys/param.h>          // MIN/MAX
#include <math.h>               // log

#include "memdmp/memdmp.h"
#include "memdmp/output.h"


// Print a classic memory dump with configurable base and grouping.
void memdump(FILE* file, const void* data, size_t length, uint8_t base, uint8_t width, uint8_t groups, unsigned mode)
{
    // Length of a line
    unsigned line_width = width * groups;

    uint64_t offset     = 0;
    while (length > offset) {
        // Starting position for the line
        const uint8_t* line = &((uint8_t*)data)[offset];

        if (((offset - line_width) > 0) && ((offset + line_width) < length)) {
            const uint8_t* prevLine = &((uint8_t*)data)[offset - line_width];
            const uint8_t* nextLine = &((uint8_t*)data)[offset + line_width];

            // Do we match the previous line?
            if (memcmp(prevLine, line, line_width) == 0) {
                // How about the next one?
                if (memcmp(nextLine, line, line_width) == 0) {
                    // Great! Are we the first one to do this?
                    if (memcmp((prevLine - line_width), line, line_width) != 0) {
                        if (mode & DUMP_ADDRESS) fprintf(file, "%12s", "");
                        if (mode & DUMP_OFFSET)  fprintf(file, "%10s", "");
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
        if (mode & DUMP_OFFSET)  fprintf(file, "%#10jx", (intmax_t)offset);

        // Print numeric representation
        if (mode & DUMP_ENCODED) {
            fprintf(file, " ");
            for (size_t c = 0; c < lineMax; c++) {
                size_t size = memstr(NULL, base, &line[c], 1, 0);
                if (size) {
                    if ((c % width) == 0) fprintf(file, " ");
                    char group[100] = "";
                    if (line[c] == 0x00) {
                        memset(group, '0', size);
                        _print_gray(file, 5, 0);
                    } else if (line[c] == 0xff) {
                        memstr(group, base, &line[c], 1, size);
                        _print_color(file, 1, 1, 3, 0);
                    } else {
                        memstr(group, base, &line[c], 1, size);
                        _print_color(file, 2, 3, 5, 0);
                    }

                    fprintf(file, "%s%s", group, ((mode & DUMP_PADDING) ? " " : ""));

                    _print_reset(file);

                } else {
                    break; // for - clearly we've run out of input.
                }
            }
        }

        // Print ASCII representation
        if (mode & DUMP_ASCII) {
            fprintf(file, " |");
            for (unsigned c = 0; c < lineMax; c++) {
                uint8_t chr = line[c] & 0xFF;
                if (chr == 0)
                    chr = ' ';
                else if (chr > 127) // ASCII unprintable
                    chr = '.';
                else if (chr < 32)  // ASCII control characters
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

