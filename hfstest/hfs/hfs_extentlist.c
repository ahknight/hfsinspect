//
//  hfs_extentlist.c
//  hfsinspect
//
//  Created by Adam Knight on 6/6/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs_extentlist.h"
#include "range.h"

ExtentList* extentlist_alloc(void)
{
    ExtentList *retval = malloc(sizeof(ExtentList));
    retval->tqh_first = NULL;
    retval->tqh_last = &retval->tqh_first; // Ensure this is pointing to the value on the heap, not the stack.
    TAILQ_INIT(retval);
    return retval;
}

void extentlist_add(ExtentList *list, size_t startBlock, size_t blockCount)
{
    if (blockCount == 0) return;
    
    Extent *newExtent = malloc(sizeof(Extent));
    newExtent->startBlock = startBlock;
    newExtent->blockCount = blockCount;
    
    Extent *existingExtent = NULL;
    
    Extent *tmp = NULL;
    TAILQ_FOREACH_REVERSE_SAFE(existingExtent, list, _ExtentList, extents, tmp) {
        if (existingExtent != NULL && existingExtent->startBlock < startBlock) {
            break;
        }
//        if (existingExtent != NULL && existingExtent->startBlock == startBlock) {
//            // Updating an entry.  Must recalc logical starts later.
//            TAILQ_REMOVE(list, existingExtent, extents);
//            free(existingExtent);
//            existingExtent = NULL;
//            break;
//        }
    }
    
    if(existingExtent == NULL) {
        TAILQ_INSERT_HEAD(list, newExtent, extents);
        newExtent->logicalStart = 0;
    } else {
        TAILQ_INSERT_AFTER(list, existingExtent, newExtent, extents);
        
        int block = 0;
        Extent *extent = NULL;
        TAILQ_FOREACH(extent, list, extents) {
            extent->logicalStart = block;
            block += extent->blockCount;
        }
    }
}

void extentlist_add_descriptor(ExtentList *list, const HFSPlusExtentDescriptor d)
{
    extentlist_add(list, d.startBlock, d.blockCount);
}

void extentlist_add_record(ExtentList *list, const HFSPlusExtentRecord r)
{
    for(int i = 0; i < kHFSPlusExtentDensity; i++) {
        extentlist_add_descriptor(list, r[i]);
    }
}

bool extentlist_find(ExtentList* list, size_t logical_block, size_t* offset, size_t* length)
{
    Extent *extent = NULL;
    TAILQ_FOREACH(extent, list, extents) {
        range lrange = make_range(extent->logicalStart, extent->blockCount);
        if (range_contains(logical_block, lrange)) {
            break;
        }
    }
    
    if (extent == NULL) {
        info("Extent for logical block %d not found.", logical_block);
        return false;
    }
    
    // Block offset within the extent; first block of request.
    size_t extentOffset = (logical_block - extent->logicalStart);
    
    // Verify there are blocks after the first.
    if ( (int)(extent->blockCount - extentOffset) <= 0 ) {
        warning("Candidate extent for logical block %d was too short.", logical_block);
        return false;
    }
    
    *offset = extent->startBlock + extentOffset;
    *length = extent->blockCount - extentOffset;
    return true;
}

void extentlist_free(ExtentList* list)
{
    Extent *e = NULL;
    while ( (e = TAILQ_FIRST(list)) ) {
        TAILQ_REMOVE(list, e, extents);
        free(e);
    }
}
