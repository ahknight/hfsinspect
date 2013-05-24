//
//  cfile.h
//  hfstest
//
//  Created by Adam Knight on 5/23/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfstest_cfile_h
#define hfstest_cfile_h

#include "stream.h"

extern stream_ops cfile_ops;

int init_cfile_stream(STREAM *stream, char* path, char* mode);

#endif
