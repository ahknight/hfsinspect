//
//  cache.h
//  hfsinspect
//
//  Created by Adam Knight on 11/17/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

typedef uint64_t ckey_t;

struct _Cache;
typedef struct _Cache* Cache;

int  cache_init(Cache* cache, uint64_t count);
void cache_destroy(Cache cache);

/**
   Finds a cache record with the given key.
   @return -1 on error, 0 on not found, 1 on found.
 */
int cache_get(Cache cache, void* buf, size_t len, ckey_t key);

/**
   Sets a cache record.
   @return -1 on error, 0 on success.
 */
int cache_set(Cache cache, const void* buf, size_t len, ckey_t key);

/**
   Zeros out the cache record.
   @return -1 on error, 0 on success.
 */
int cache_rem(Cache cache, ckey_t key);
