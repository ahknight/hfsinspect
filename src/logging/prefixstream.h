//
//  prefixstream.h
//  hfsinspect
//
//  Created by Adam Knight on 11/30/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_prefixstream_h
#define hfsinspect_prefixstream_h

#include <stdio.h>

FILE* prefixstream(FILE* fp, char* prefix) __attribute__((nonnull));

#endif
