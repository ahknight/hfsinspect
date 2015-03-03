//
//  hfs_endian.h
//  hfsinspect
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_endian_h
#define hfsinspect_hfs_endian_h

#include "hfs/types.h"
#include "volumes/_endian.h"

#pragma mark HFS

void swap_HFSMasterDirectoryBlock   (HFSMasterDirectoryBlock* record) __attribute__((nonnull));
void swap_HFSExtentRecord           (HFSExtentRecord* record) __attribute__((nonnull));
void swap_HFSExtentDescriptor       (HFSExtentDescriptor* record) __attribute__((nonnull));

#pragma mark HFS Plus

void swap_HFSPlusVolumeHeader       (HFSPlusVolumeHeader* vh) __attribute__((nonnull));
void swap_JournalInfoBlock          (JournalInfoBlock* record) __attribute__((nonnull));
void swap_HFSPlusForkData           (HFSPlusForkData* fork) __attribute__((nonnull));
void swap_HFSUniStr255              (HFSUniStr255* unistr) __attribute__((nonnull));
void swap_HFSPlusExtentRecord       (HFSPlusExtentDescriptor record[]) __attribute__((nonnull));
void swap_HFSPlusExtentDescriptor   (HFSPlusExtentDescriptor* extent) __attribute__((nonnull));

void swap_HFSPlusBSDInfo            (HFSPlusBSDInfo* record) __attribute__((nonnull));
void swap_FndrDirInfo               (FndrDirInfo* record) __attribute__((nonnull));
void swap_FndrFileInfo              (FndrFileInfo* record) __attribute__((nonnull));
void swap_FndrOpaqueInfo            (FndrOpaqueInfo* record) __attribute__((nonnull));
void swap_HFSPlusCatalogKey         (HFSPlusCatalogKey* key) __attribute__((nonnull));
void swap_HFSPlusCatalogRecord      (HFSPlusCatalogRecord* record) __attribute__((nonnull));
void swap_HFSPlusCatalogFile        (HFSPlusCatalogFile* record) __attribute__((nonnull));
void swap_HFSPlusCatalogFolder      (HFSPlusCatalogFolder* record) __attribute__((nonnull));
void swap_HFSPlusCatalogThread      (HFSPlusCatalogThread* record) __attribute__((nonnull));

#endif
