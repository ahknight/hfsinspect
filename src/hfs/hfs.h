//
//  hfs.h
//  hfsinspect
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_h
#define hfsinspect_hfs_h

#include "hfs/types.h"

#include "hfs/hfs_io.h"
#include "hfs/hfs_endian.h"

#include "hfs/extents.h"
#include "hfs/catalog.h"
#include "hfs/btree/btree.h"
#include "hfs/Apple/hfs_types.h"

#pragma mark Struct Conveniences

int     hfs_test (Volume* vol) __attribute__((nonnull));
Volume* hfsplus_find (Volume* vol) __attribute__((nonnull));

int hfs_open (HFSPlus* hfs, Volume* vol) __attribute__((nonnull));
int hfs_close (HFSPlus* hfs) __attribute__((nonnull));

bool hfs_get_HFSMasterDirectoryBlock(HFSMasterDirectoryBlock* vh, const HFSPlus* hfs) __attribute__((nonnull));
bool hfsplus_get_JournalInfoBlock       (JournalInfoBlock* block, const HFSPlus* hfs) __attribute__((nonnull));
bool hfsplus_get_journalheader          (journal_header* header, JournalInfoBlock* info, const HFSPlus* hfs) __attribute__((nonnull));

#endif
