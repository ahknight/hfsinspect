//
//  hfs.h
//  hfsinspect
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_h
#define hfsinspect_hfs_h

#include "hfs/hfs_types.h"
#include "btree/btree.h"

#include "hfs/hfs_io.h"
#include "hfs/hfs_endian.h"

#include "hfs/extents.h"
#include "hfs/catalog.h"
#include "hfs/hfsplus/hotfiles.h"
#include "hfs/hfsplus/attributes.h"
#include "hfs/Apple/vfs_journal.h"

#pragma mark Struct Conveniences

int     hfs_test    (Volume* vol)               _NONNULL;
Volume* hfs_find    (Volume* vol)               _NONNULL;

int     hfs_attach  (HFS* hfs, Volume* vol)     _NONNULL;
int     hfs_close   (HFS* hfs)                  _NONNULL;

bool    hfs_get_HFSMasterDirectoryBlock(HFSMasterDirectoryBlock* vh, const HFS* hfs)                    _NONNULL;
bool    hfs_get_HFSPlusVolumeHeader    (HFSPlusVolumeHeader* vh, const HFS* hfs)                        _NONNULL;
bool    hfs_get_JournalInfoBlock       (JournalInfoBlock* block, const HFS* hfs)                        _NONNULL;
bool    hfs_get_journalheader          (journal_header *header, JournalInfoBlock info, const HFS* hfs)  _NONNULL;

#endif
