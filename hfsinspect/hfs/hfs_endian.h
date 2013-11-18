//
//  hfs_endian.h
//  hfsinspect
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_endian_h
#define hfsinspect_hfs_endian_h

#include "hfs_structs.h"

#pragma mark HFS

void swap_HFSMasterDirectoryBlock   (HFSMasterDirectoryBlock* record);
void swap_HFSExtentRecord           (HFSExtentRecord* record);
void swap_HFSExtentDescriptor       (HFSExtentDescriptor* record);

#pragma mark HFS Plus

void swap_HFSPlusVolumeHeader       (HFSPlusVolumeHeader *vh);
void swap_JournalInfoBlock          (JournalInfoBlock* record);
void swap_HFSPlusForkData           (HFSPlusForkData *fork);
void swap_HFSPlusExtentRecord       (HFSPlusExtentDescriptor record[]);
void swap_HFSPlusExtentDescriptor   (HFSPlusExtentDescriptor *extent);
void swap_BTNodeDescriptor          (BTNodeDescriptor *node);
void swap_BTHeaderRec               (BTHeaderRec *header);
void swap_HFSPlusCatalogKey         (HFSPlusCatalogKey *key);
void swap_HFSUniStr255              (HFSUniStr255 *unistr);
void swap_HFSPlusCatalogFolder      (HFSPlusCatalogFolder *record);
void swap_HFSPlusBSDInfo            (HFSPlusBSDInfo *record);
void swap_FndrDirInfo               (FndrDirInfo *record);
void swap_FndrFileInfo              (FndrFileInfo *record);
void swap_FndrOpaqueInfo            (FndrOpaqueInfo *record);
void swap_HFSPlusCatalogFile        (HFSPlusCatalogFile *record);
void swap_HFSPlusCatalogThread      (HFSPlusCatalogThread *record);
int  swap_BTreeNode                 (BTreeNode *node);

#endif
