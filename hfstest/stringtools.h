//
//  stringtools.h
//  hfstest
//
//  Created by Adam Knight on 6/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfstest_stringtools_h
#define hfstest_stringtools_h

// repeat character
char* repchar(char c, int count);

// repeat wide character
wchar_t* repwchar(wchar_t c, int count);

// Generate a human-readable string representing file size
char* sizeString(size_t size);

// Generate a string representation of a buffer with adjustable number base (2-36).
char* memstr(const char* buf, size_t length, u_int8_t base);

// Generate a hexadecimal representation of a buffer.
char* hrep(const char* buf, size_t length);

// Print a classic memory dump with configurable base and grouping.
void memdump(const char* data, size_t length, u_int8_t base, u_int8_t gsize, u_int8_t gcount);

#endif
