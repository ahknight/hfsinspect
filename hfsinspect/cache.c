//
//  cache.c
//  hfsinspect
//
//  Created by Adam Knight on 11/17/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "cache.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <search.h>
#include <sys/param.h>
#include <stdio.h>
#include <errno.h>

#include <malloc/malloc.h>


typedef struct CacheRecord CacheRecord;
typedef CacheRecord * CacheRecordPtr;

struct _Cache {
    uint64_t        max_records;
    uint64_t        record_count;
    void*           index;
    CacheRecordPtr  records;
} __attribute__((aligned(2), packed));

struct CacheRecord {
    CacheRecordPtr  next;
    CacheRecordPtr  prev;
    
    ckey_t      key;
    size_t      datalen;
    char        *data;
} __attribute__((aligned(2), packed));


#define ASSERT_PTR(ptr) if (ptr == NULL) { errno = EINVAL; return -1; }
#define esizeof(st, m) sizeof(((st*)(NULL))->m)

#pragma mark Comparators

#define cmp(a, b) ((a) > (b) ? 1 : ((a) < (b) ? -1 : 0))

int key_compare(const void* a, const void* b)
{
    if (a == NULL) return -1;
    if (b == NULL) return 1;
    
    return cmp( ((CacheRecordPtr)a)->key, ((CacheRecordPtr)b)->key );
}

#pragma mark Internal

CacheRecordPtr  cache_record_get_(Cache cache, ckey_t key);
CacheRecordPtr  cache_record_new_(Cache cache);
int             cache_record_remove_(Cache cache, CacheRecordPtr* record);

/**
 Finds a cache record with the given key.
 @return -1 on error, 0 on not found, 1 on found.
 */
CacheRecordPtr cache_record_get_(Cache cache, ckey_t key)
{
    if (cache == NULL) return NULL;
    
    CacheRecord keyRec = { .key = key };
    
    void* rec = tfind(&keyRec, &cache->index, key_compare);
    if (rec == NULL) return NULL;
    
    CacheRecordPtr record = *(CacheRecordPtr*)rec;
    return record;
}

CacheRecordPtr cache_record_new_(Cache cache)
{
    if (cache == NULL) return NULL;
    
    // Create and insert a new record.
    CacheRecordPtr record = calloc(1, sizeof(CacheRecord));
    if (record == NULL) return NULL;
    
    insque(record, cache->records);
    if (cache->records == NULL) {
        cache->records = record;
        
    } else if (cache->record_count > cache->max_records) {
        // Remove the LRU record if we're over the limit.
        CacheRecordPtr r = cache->records;
        while (r->next != NULL) r = r->next;
        cache_record_remove_(cache, &r);
    }
    
    cache->record_count++;
    
    return record;
}

/**
 Zeros out the cache record.
 @return -1 on error, 0 on success.
 */
int cache_record_remove_(Cache cache, CacheRecordPtr* r)
{
    ASSERT_PTR(cache);
    ASSERT_PTR(r);
    
    CacheRecordPtr record = *r;
    
    if (malloc_size(record) > 0) {
        if (cache->record_count > 0) cache->record_count--;
        
        if (tfind(record, &cache->index, key_compare) != NULL)
            tdelete(record, &cache->index, key_compare);
        
        remque(record);
        
        if (malloc_size(record->data) > 0) {
            memset(record->data, 0, malloc_size(record->data));
            free(record->data);
        }
        
        memset(record, 0, malloc_size(record));
        free(record);
        
        *r = NULL;
    }
    
    return 0;
}


#pragma mark API

int cache_init(Cache* cache, uint64_t num_records)
{
    ASSERT_PTR(cache);
    
    *cache = calloc(1, sizeof(struct _Cache));
    if (*cache == NULL) return -1;
    
    Cache c = *cache;
    c->max_records = num_records;
    c->record_count = 0;
    c->records = calloc(1, sizeof(CacheRecord));
    
    return 0;
}

void cache_destroy(Cache cache)
{
    if (cache == NULL) { return; }
    
    // Clean up all the records
    CacheRecordPtr r = cache->records;
    CacheRecordPtr n = NULL;
    while ( (n = r->next) ) {
        cache_record_remove_(cache, &r);
        r = n;
    }

    cache_record_remove_(cache, &cache->records);
    
    free(cache);
}

int cache_get(Cache cache, char* buf, size_t len, ckey_t key)
{
    ASSERT_PTR(cache);
    ASSERT_PTR(buf);
    
    CacheRecordPtr record = NULL;
    
    if ( (record = cache_record_get_(cache, key)) == NULL)
        return 0;
    
    // Move the record to the front of the list since it's been used.
    remque(record);
    insque(record, cache->records);
    
    if( (buf != NULL) && (len > 0) ) {
        len = MIN(record->datalen, len);
        memcpy(buf, record->data, len);
    }
    
    return 1;
}

int cache_set(Cache cache, const char* buf, size_t len, ckey_t key)
{
    ASSERT_PTR(cache);
    ASSERT_PTR(buf);
    
    // If there is an existing record with this key, delete it.
    cache_rem(cache, key);
    
    CacheRecordPtr record = NULL;
    if ( (record = cache_record_new_(cache)) == NULL) goto ERROR;
    
    record->key = key;
    record->datalen = len;
    if ( (record->data = calloc(1, record->datalen)) == NULL) goto ERROR;
    memcpy(record->data, buf, record->datalen);
    
    void* ref = NULL;
    if ( (ref = tsearch(record, &cache->index, key_compare)) == NULL ) goto ERROR;
    
    return 0;
    
ERROR:
    if (errno != ENOMEM) errno = ENOBUFS;
    return -1;
}

int cache_rem(Cache cache, ckey_t key)
{
    CacheRecordPtr record = NULL;
    if ( (record = cache_record_get_(cache, key)) != NULL)
        cache_record_remove_(cache, &record);
    
    return 0;
}
