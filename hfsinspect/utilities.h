//
//  utilities.h
//  hfsinspect
//
//  Created by Adam Knight on 11/26/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_utilities_h
#define hfsinspect_utilities_h

ssize_t fpread                  (FILE* f, void* buf, size_t nbytes, off_t offset);

static inline int icmp(uint64_t a, uint64_t b) {
    return (a > b ? 1 : (a < b ? -1 : 0));
}

static inline int cmp(const void * a, const void * b) {
    return (a > b ? 1 : (a < b ? -1 : 0));
}

#endif
