//
//  hfs_endian.h
//  hfsinspect
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_endian_h
#define hfsinspect_hfs_endian_h

#include "hfs/hfs_structs.h"

#pragma mark HFS

void swap_HFSMasterDirectoryBlock   (HFSMasterDirectoryBlock* record);
void swap_HFSExtentRecord           (HFSExtentRecord* record);
void swap_HFSExtentDescriptor       (HFSExtentDescriptor* record);

#pragma mark HFS Plus

void swap_HFSPlusVolumeHeader       (HFSPlusVolumeHeader *vh);
void swap_JournalInfoBlock          (JournalInfoBlock* record);
void swap_HFSPlusForkData           (HFSPlusForkData *fork);
void swap_HFSUniStr255              (HFSUniStr255 *unistr);
void swap_HFSPlusExtentRecord       (HFSPlusExtentDescriptor record[]);
void swap_HFSPlusExtentDescriptor   (HFSPlusExtentDescriptor *extent);

#endif
