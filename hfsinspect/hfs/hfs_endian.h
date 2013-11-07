//
//  hfs_endian.h
//  hfsinspect
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_endian_h
#define hfsinspect_hfs_endian_h

#include <libkern/OSByteOrder.h>
#include "hfs_structs.h"
#include "partitions.h"

// Define macros to convert big endian ints to host order.
// (Defined as simple casts on little endian systems.)
#if defined(__LITTLE_ENDIAN__)

#define S16(__a) 		OSSwapBigToHostInt16 (__a)
#define S32(__a) 		OSSwapBigToHostInt32 (__a)
#define S64(__a) 		OSSwapBigToHostInt64 (__a)

#elif defined(__BIG_ENDIAN__)

#define S16(__a) 		((u_int16_t)(__a))
#define S32(__a) 		((u_int32_t)(__a))
#define S64(__a) 		((u_int64_t)(__a))

#endif

#pragma mark Apple Partition Map

void swap_APMHeader                 (APMHeader* record);

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
int  swap_BTreeNode                 (HFSBTreeNode *node);

#endif
