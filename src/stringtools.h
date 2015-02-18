//
//  stringtools.h
//  hfsinspect
//
//  Created by Adam Knight on 6/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_stringtools_h
#define hfsinspect_stringtools_h

//#include <stdio.h>          // FILE*
//#include <stdint.h>         // uint*

enum {
    DUMP_ADDRESS = 0x0001,
    DUMP_OFFSET  = 0x0002,
    DUMP_ENCODED = 0x0004,
    DUMP_ASCII   = 0x0008,
    DUMP_FULL    = 0x00FF,

    DUMP_PADDING = 0x0100,
};

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
void memdump(FILE* file, const void* data, size_t length, uint8_t base, uint8_t width, uint8_t groups, unsigned mode);

#endif
