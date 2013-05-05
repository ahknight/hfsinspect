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

void PrintVolumeHeader(HFSPlusVolumeHeader *vh);
void PrintBTNodeDescriptor(BTNodeDescriptor *nd);
void PrintBTHeaderRecord(BTHeaderRec *hr);

#endif
