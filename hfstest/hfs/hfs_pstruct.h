//
//  hfs_pstruct.h
//  hfstest
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfstest_hfs_pstruct_h
#define hfstest_hfs_pstruct_h

#include "hfs_structs.h"

#define MAC_GMT_FACTOR 2082844800UL

void set_hfs_volume                 (HFSVolume *v);

void PrintVolumeHeader              (const HFSPlusVolumeHeader *vh);
void PrintHFSPlusExtentRecordBuffer (const Buffer *buffer, unsigned int count, u_int32_t totalBlocks);
void PrintHFSPlusForkData           (const HFSPlusForkData *fork, u_int32_t cnid, u_int8_t forktype);

void PrintBTNodeDescriptor          (const BTNodeDescriptor *nd);
void PrintBTHeaderRecord            (const BTHeaderRec *hr);

void PrintHFSPlusCatalogFolder      (const HFSPlusCatalogFolder *record);
void PrintHFSPlusCatalogFile        (const HFSPlusCatalogFile *record);
void PrintHFSPlusCatalogThread      (const HFSPlusCatalogThread *record);

void VisualizeData                  (const char* data, size_t length);
void VisualizeHFSPlusExtentKey      (const HFSPlusExtentKey *record, const char* label, bool oneLine);
void VisualizeHFSPlusCatalogKey     (const HFSPlusCatalogKey *record, const char* label, bool oneLine);

void PrintExtentsSummary            (const HFSFork* fork);

void PrintNode                      (const HFSBTreeNode* node);
void PrintNodeRecord                (const HFSBTreeNode* node, int recordNumber);
void PrintTreeNode                  (const HFSBTree *tree, u_int32_t nodeID);

void VisualizeHFSBTreeNodeRecord    (const HFSBTreeNodeRecord* record, const char* label);

#endif
