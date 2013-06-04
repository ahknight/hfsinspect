//
//  hfs_plus.h
//  hfstest
//
//  Created by Adam Knight on 5/29/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfstest_hfs_plus_h
#define hfstest_hfs_plus_h

#include "udio.h"
#include <stdbool.h>
#include <hfs/hfs_format.h>

#ifndef	MIN
#define	MIN(a, b)	((a) < (b) ? (a) : (b))
#endif

#ifndef	MAX
#define	MAX(a, b)	((a) > (b) ? (a) : (b))
#endif

#define MAC_GMT_FACTOR      2082844800UL

#pragma mark UDIO Structures

struct hfs_plus_volume_ops;
typedef struct hfs_plus_volume_ops hfs_plus_volume_ops;

typedef struct hfs_plus_volume_opts {
    char    *path;
} hfs_plus_volume_opts;


struct hfs_plus_fork_ops;
typedef struct hfs_plus_fork_ops hfs_plus_fork_ops;

struct hfs_fork_opts {
    udio_t              hfs;        // Volume to read from.
    HFSPlusForkData     fork_data;  // Contains size info and first extents.
    u_int32_t           cnid;       // Catalog node ID.
    u_int8_t            fork_type;  // Fork (HFSDataForkType or 0xFF)
};
typedef struct hfs_fork_opts hfs_fork_opts;



#pragma mark Range Helpers

typedef struct range {
    off_t   start;
    size_t  count;
} range;
typedef range* range_ptr;

range   make_range          (off_t start, size_t count);
bool    range_contains      (off_t pos, range test);
size_t  range_max           (range r);
range   range_intersection  (range a, range b);

#endif
