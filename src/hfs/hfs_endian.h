//
//  hfs_endian.h
//  hfsinspect
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_endian_h
#define hfsinspect_hfs_endian_h

#include "hfs/hfs_types.h"

#pragma mark HFS

void swap_HFSMasterDirectoryBlock   (HFSMasterDirectoryBlock* record) _NONNULL;
void swap_HFSExtentRecord           (HFSExtentRecord* record) _NONNULL;
void swap_HFSExtentDescriptor       (HFSExtentDescriptor* record) _NONNULL;

#pragma mark HFS Plus

void swap_HFSPlusVolumeHeader       (HFSPlusVolumeHeader *vh) _NONNULL;
void swap_JournalInfoBlock          (JournalInfoBlock* record) _NONNULL;
void swap_HFSPlusForkData           (HFSPlusForkData *fork) _NONNULL;
void swap_HFSUniStr255              (HFSUniStr255 *unistr) _NONNULL;
void swap_HFSPlusExtentRecord       (HFSPlusExtentDescriptor record[]) _NONNULL;
void swap_HFSPlusExtentDescriptor   (HFSPlusExtentDescriptor *extent) _NONNULL;

#endif
