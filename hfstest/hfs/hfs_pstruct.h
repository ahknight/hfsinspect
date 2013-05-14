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

void PrintVolumeHeader(HFSPlusVolumeHeader *vh);


void PrintHFSPlusForkData(HFSPlusForkData *fork);


void PrintBTNodeDescriptor(BTNodeDescriptor *nd);

void PrintBTHeaderRecord(BTHeaderRec *hr);


void PrintCatalogExcerpt(HFSVolume *hfs, int numNodes);

void PrintCatalogNode(BTreeNode *node);

void PrintHFSPlusCatalogFolder(HFSPlusCatalogFolder *record);

void PrintHFSPlusCatalogFile(HFSPlusCatalogFile *record);

void PrintHFSPlusCatalogThread(HFSPlusCatalogThread *record);

void PrintHFSPlusCatalogKey(HFSPlusCatalogKey *record);

#endif
