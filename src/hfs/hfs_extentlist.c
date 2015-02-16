//
//  hfs_extentlist.c
//  hfsinspect
//
//  Created by Adam Knight on 6/6/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <errno.h>              // errno/perror

#include "hfsinspect/range.h"
#include "hfs/hfs_extentlist.h"
#include "hfs/output_hfs.h"
#include "logging/logging.h"    // console printing routines


#if defined(__linux__)
// Copied from OS X's sys/queue.h. Covered by the APSL 2.0 and/or the original BSD license (ie. "Copyright (c) 1991, 1993 The Regents of the University of California.  All rights reserved." etc.)

#define TAILQ_FOREACH_REVERSE_SAFE(var, head, headname, field, tvar)    \
    for ((var) = TAILQ_LAST((head), headname);          \
         (var) && ((tvar) = TAILQ_PREV((var), headname, field), 1);  \
         (var) = (tvar))

#endif

ExtentList* extentlist_make(void)
{
    trace("%s", __PRETTY_FUNCTION__);
    ExtentList* retval = NULL;
    SALLOC(retval, sizeof(ExtentList));
    retval->tqh_first = NULL;
    retval->tqh_last  = &retval->tqh_first; // Ensure this is pointing to the value on the heap, not the stack.
    TAILQ_INIT(retval);
    return retval;
}

void extentlist_add(ExtentList* list, size_t startBlock, size_t blockCount)
{
    Extent* newExtent      = NULL;
    Extent* existingExtent = NULL;
    Extent* tmp            = NULL;

    // trace("list (%p), startBlock %zu, blockCount %zu", list, startBlock, blockCount);

    if (blockCount == 0) return;

    SALLOC(newExtent, sizeof(Extent));
    newExtent->startBlock = startBlock;
    newExtent->blockCount = blockCount;

    TAILQ_FOREACH_REVERSE_SAFE(existingExtent, list, _ExtentList, extents, tmp) {
        if ((existingExtent != NULL) && (existingExtent->startBlock < startBlock)) {
            break;
        }
//        if (existingExtent != NULL && existingExtent->startBlock == startBlock) {
//            // Updating an entry.  Must recalc logical starts later.
//            TAILQ_REMOVE(list, existingExtent, extents);
//            SFREE(existingExtent);
//            existingExtent = NULL;
//            break;
//        }
    }

    if(existingExtent == NULL) {
        TAILQ_INSERT_HEAD(list, newExtent, extents);
        newExtent->logicalStart = 0;
    } else {
        TAILQ_INSERT_AFTER(list, existingExtent, newExtent, extents);

        int     block  = 0;
        Extent* extent = NULL;

        // For large files, this takes a LONG time. Optimize to only do this when inserting in the middle.
        TAILQ_FOREACH(extent, list, extents) {
            extent->logicalStart = block;
            block               += extent->blockCount;
        }
    }
}

void extentlist_add_descriptor(ExtentList* list, const HFSPlusExtentDescriptor d)
{
    // trace("list (%p), d (%u, %u)", list, d.startBlock, d.blockCount);
    extentlist_add(list, d.startBlock, d.blockCount);
}

void extentlist_add_record(ExtentList* list, const HFSPlusExtentRecord r)
{
    // trace("list (%p), r[]", list);
    for(unsigned i = 0; i < kHFSPlusExtentDensity; i++) {
        extentlist_add_descriptor(list, r[i]);
    }
}

bool extentlist_find(ExtentList* list, size_t logical_block, size_t* offset, size_t* length)
{
    Extent* extent = NULL;

    trace("list (%p), logical_block %zu, offset (%p), length (%p)", list, logical_block, offset, length);

    TAILQ_FOREACH(extent, list, extents) {
        range lrange = make_range(extent->logicalStart, extent->blockCount);
        if (range_contains(logical_block, lrange)) {
            break;
        }
    }

    if (extent == NULL) {
//        debug("Extent for logical block %zu not found.", logical_block);
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
    Extent* e = NULL;

    trace("list (%p)", list);

    while ( (e = TAILQ_FIRST(list)) ) {
        TAILQ_REMOVE(list, e, extents);
        SFREE(e);
    }
    SFREE(list);
}

