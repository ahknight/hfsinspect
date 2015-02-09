//
//  cache.h
//  hfsinspect
//
//  Created by Adam Knight on 11/17/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <sys/types.h>
#include <stdint.h>

typedef uint64_t ckey_t;

struct _Cache;
typedef struct _Cache* Cache;

int  cache_init(Cache* cache, uint64_t count);
void cache_destroy(Cache cache);

int cache_get(Cache cache,       char* buf, size_t len, ckey_t key);
int cache_set(Cache cache, const char* buf, size_t len, ckey_t key);
int cache_rem(Cache cache, ckey_t key);
