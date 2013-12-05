//
//  hfs_extentlist.c
//  hfsinspect
//
//  Created by Adam Knight on 6/6/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "misc/range.h"
#include "hfs/hfs_extentlist.h"

#if defined(__linux__) //sucks

// Copied from OS X's sys/queue.h. Covered by the APSL 2.0 and/or the original BSD license (ie. "Copyright (c) 1991, 1993 The Regents of the University of California.  All rights reserved." etc.)

#define	TAILQ_EMPTY(head)	((head)->tqh_first == NULL)
#define	TAILQ_FIRST(head)	((head)->tqh_first)
#define	TAILQ_FOREACH_REVERSE_SAFE(var, head, headname, field, tvar)	\
	for ((var) = TAILQ_LAST((head), headname);			\
	    (var) && ((tvar) = TAILQ_PREV((var), headname, field), 1);	\
	    (var) = (tvar))
#define	TAILQ_LAST(head, headname)					\
	(*(((struct headname *)((head)->tqh_last))->tqh_last))
#define	TAILQ_NEXT(elm, field) ((elm)->field.tqe_next)
#define	TAILQ_PREV(elm, headname, field)				\
	(*(((struct headname *)((elm)->field.tqe_prev))->tqh_last))

#endif

ExtentList* extentlist_make(void)
{
    ExtentList *retval;
    ALLOC(retval, sizeof(ExtentList));
    retval->tqh_first = NULL;
    retval->tqh_last = &retval->tqh_first; // Ensure this is pointing to the value on the heap, not the stack.
    TAILQ_INIT(retval);
    return retval;
}

void extentlist_add(ExtentList *list, size_t startBlock, size_t blockCount)
{
    if (blockCount == 0) return;
    
    Extent *newExtent;
    ALLOC(newExtent, sizeof(Extent));
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
        
        // For large files, this takes a LONG time. Optimize to only do this when inserting in the middle.
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
        info("Extent for logical block %zu not found.", logical_block);
        return false;
    }
    
    // Block offset within the extent; first block of request.
    size_t extentOffset = (logical_block - extent->logicalStart);
    
    // Verify there are blocks after the first.
    if ( (int)(extent->blockCount - extentOffset) <= 0 ) {
        warning("Candidate extent for logical block %zu was too short.", logical_block);
        return false;
    }
    
    if (offset != NULL) *offset = extent->startBlock + extentOffset;
    if (length != NULL) *length = extent->blockCount - extentOffset;
    return true;
}

void extentlist_free(ExtentList* list)
{
    Extent *e = NULL;
    while ( (e = TAILQ_FIRST(list)) ) {
        TAILQ_REMOVE(list, e, extents);
        FREE(e);
    }
    FREE(list);
}
