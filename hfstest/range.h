//
//  range.h
//  hfstest
//
//  Created by Adam Knight on 6/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfstest_range_h
#define hfstest_range_h

#pragma mark Range Helpers

typedef struct range {
    size_t  start;
    size_t  count;
} range;
typedef range* range_ptr;

range   make_range          (size_t start, size_t count);
bool    range_contains      (size_t pos, range test);
size_t  range_max           (range r);
range   range_intersection  (range a, range b);

#endif
