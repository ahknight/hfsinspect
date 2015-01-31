//
//  hfs_extentlist.h
//  hfsinspect
//
//  Created by Adam Knight on 6/6/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_extentlist_h
#define hfsinspect_hfs_extentlist_h

#if defined(__linux__)
    #include <bsd/sys/queue.h>  // TAILQ
#else
    #include <sys/queue.h>
#endif

#include "hfs/types.h"

typedef struct _Extent {
    size_t logicalStart;
    size_t startBlock;
    size_t blockCount;
    TAILQ_ENTRY(_Extent) extents;
} Extent;

typedef TAILQ_HEAD(_ExtentList, _Extent) ExtentList;

// Creates the tailq head node.
ExtentList* extentlist_make            (void);

// Adds a node.
void        extentlist_add              (ExtentList *list, size_t startBlock, size_t blockCount) __attribute__((nonnull));

// Calls _add.
void        extentlist_add_descriptor   (ExtentList *list, const HFSPlusExtentDescriptor d) __attribute__((nonnull));
void        extentlist_add_record       (ExtentList *list, const HFSPlusExtentRecord r) __attribute__((nonnull));

// Returns true/false if the logical block exists in the list and returns the corresponding allocation block and block run available after it in the extent.
bool        extentlist_find             (ExtentList *list, size_t logical_block, size_t* offset, size_t* length) __attribute__((nonnull(1)));

// Removes and frees all the nodes.
void        extentlist_free             (ExtentList* list) __attribute__((nonnull));

#endif
