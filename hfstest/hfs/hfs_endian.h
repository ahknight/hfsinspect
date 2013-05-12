//
//  hfs_endian.h
//  hfstest
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfstest_hfs_endian_h
#define hfstest_hfs_endian_h

#include <libkern/OSByteOrder.h>


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

#endif

void swap_HFSPlusVolumeHeader(HFSPlusVolumeHeader *vh);
void swap_HFSPlusForkData(HFSPlusForkData *fork);
void swap_HFSPlusExtentDescriptor(HFSPlusExtentDescriptor *extent);
void swap_BTNodeDescriptor(BTNodeDescriptor *node);
void swap_BTHeaderRec(BTHeaderRec *header);
