//
//  hfs_pstruct.h
//  hfstest
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfstest_hfs_pstruct_h
#define hfstest_hfs_pstruct_h

#include <hfs/hfs_format.h>
#include "hfs_structs.h"

void PrintVolumeHeader(const HFSPlusVolumeHeader *vh);
void PrintHFSPlusExtentRecordBuffer(const Buffer *buffer, unsigned int count, u_int32_t totalBlocks);
void PrintHFSPlusForkData(const HFSPlusForkData *fork);

void PrintBTNodeDescriptor(const BTNodeDescriptor *nd);
void PrintBTHeaderRecord(const BTHeaderRec *hr);

void PrintHFSPlusCatalogFolder(const HFSPlusCatalogFolder *record);
void PrintHFSPlusCatalogFile(const HFSPlusCatalogFile *record);
void PrintHFSPlusCatalogThread(const HFSPlusCatalogThread *record);

void VisualizeData(const char* data, size_t length);
void VisualizeHFSPlusExtentKey(const HFSPlusExtentKey *record, const char* label);

void PrintHFSPlusExtentKey(const HFSPlusExtentKey *record);
void PrintHFSPlusCatalogKey(const HFSPlusCatalogKey *record);

void PrintExtentsSummary(const HFSFork* fork);

void PrintNode(const HFSBTreeNode* node);
void PrintNodeRecord(const HFSBTreeNode* node, int recordNumber);
void PrintTreeNode(const HFSBTree *tree, u_int32_t nodeID);

#endif
