//
//  crc32.h
//  hfsinspect
//
//  Created by Adam Knight on 3/28/14.
//  Copyright (c) 2014 Adam Knight. All rights reserved.
//

// GPT doesn't use CRC-32C; it uses the IEEE polynomial.  So here we are.

#ifndef hfsinspect_crc32_h
#define hfsinspect_crc32_h

#include <stdint.h>

uint32_t crc32 (uint32_t crc, const void *buf, size_t len);

#endif
