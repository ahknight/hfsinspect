//
//  range.c
//  hfsinspect
//
//  Created by Adam Knight on 6/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <sys/param.h>  //MIN/MAX

#include "range.h"


#pragma mark Range Helpers

range make_range(size_t start, size_t count)
{
    range r = { .start = start, .count = count };
    return r;
}

bool range_equal(range a, range b)
{
    if (a.start == b.start) {
        if (a.count == b.count) {
            return true;
        }
    }
    return false;
}

bool range_contains(size_t pos, range r)
{
    /*
       0 in 0,0 is false  (start 0; end -1)
       0 in 0,1 is true   (start 0; end 0)
       1 in 0,1 is false  (start 0; end 0)
       1 in 0,2 is true   (start 0; end 1)
       1 in 1,0 is false  (start 1; end 0)
     */
    size_t start = r.start;
    size_t end   = r.start + r.count - 1;

    return (pos >= start && pos <= end);
}

size_t range_max(range r)
{
    return (r.start + r.count);
}

range range_intersection(range a, range b)
{
    // A range is considered to intersect if the start or end is within our bounds.
    range r = {0};

    if (
        ( range_contains(a.start, b) || range_contains(range_max(a), b) ) ||
        ( range_contains(b.start, a) || range_contains(range_max(b), a) )
        ) {
        r.start = MAX(a.start, b.start);
        r.count = MIN(a.count, b.count);
    }

    return r;
}

