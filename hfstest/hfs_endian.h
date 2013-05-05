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

#if defined(__LITTLE_ENDIAN__)

#define BE16(__a) 		OSSwapBigToHostInt16 (__a)
#define BE32(__a) 		OSSwapBigToHostInt32 (__a)
#define BE64(__a) 		OSSwapBigToHostInt64 (__a)

#elif defined(__BIG_ENDIAN__)

#define BE16(__a) 		((u_int16_t)(__a))
#define BE32(__a) 		((u_int32_t)(__a))
#define BE64(__a) 		((u_int64_t)(__a))

#endif

#define MAC_GMT_FACTOR      2082844800UL



#endif
