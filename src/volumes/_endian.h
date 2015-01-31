//
//  hfs/_endian.h
//  hfsinspect
//
//  Created by Adam Knight on 11/10/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_endian_h
#define hfsinspect_endian_h

// ... and now the platform and CPU-specific macrofest.
// All we need are optimized versions of the three be##toh functions.
// Most OSes have these already.  We have good fallbacks, too.

#if defined(__APPLE__)

    #include <libkern/OSByteOrder.h>

    #define be16toh(__x)   OSSwapBigToHostInt16(__x)
    #define be32toh(__x)   OSSwapBigToHostInt32(__x)
    #define be64toh(__x)   OSSwapBigToHostInt64(__x)

#elif defined(__FreeBSD__) || defined(__NetBSD__)

    #include <sys/endian.h>

#elif defined(__linux__)

    #include <endian.h>

#else /* unknown platform; see if the compiler has a builtin or use the fallback. */

    #if defined __clang__ || defined __GNUC__ && (__GNUC__ > 4 || (__xGNUC__ == 4 && __GNUC_PATCHLEVEL__ > 8 ))
        static inline uint16_t bswap16(uint16_t x) { return __builtin_bswap16(x); }
    #else
        #warning Using fallback bswap16 (upgrade your compiler!).
        static inline uint16_t bswap16(uint16_t x) { return (uint16_t)(((x & 0x00ff) << 8)|((x & 0xff00) >> 8)); }
    #endif

    #if defined __clang__ || defined __GNUC__ && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_PATCHLEVEL__ > 3 ))
        static inline uint32_t bswap32(uint32_t x) { return __builtin_bswap32(x); }
    #else
        #warning Using fallback bswap32 (upgrade your compiler!).
        static inline uint32_t bswap32(uint32_t x)
        {
            return (uint32_t)(
                              ((x & 0x000000ffU) << 24) |
                              ((x & 0x0000ff00U) <<  8) |
                              ((x & 0x00ff0000U) >>  8) |
                              ((x & 0xff000000U) >> 24) );
        }
    #endif

    #if defined __clang__ || defined __GNUC__ && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_PATCHLEVEL__ > 3 ))
        static inline uint64_t bswap64(uint64_t x) { return __builtin_bswap64(x); }
    #else
        #warning Using fallback bswap64 (upgrade your compiler!).
        static inline uint64_t bswap64(uint64_t x)
        {
            return (uint64_t)(
                              ((x & 0x00000000000000ffULL) << 56) |
                              ((x & 0x000000000000ff00ULL) << 40) |
                              ((x & 0x0000000000ff0000ULL) << 24) |
                              ((x & 0x00000000ff000000ULL) <<  8) |
                              ((x & 0x000000ff00000000ULL) >>  8) |
                              ((x & 0x0000ff0000000000ULL) >> 24) |
                              ((x & 0x00ff000000000000ULL) >> 40) |
                              ((x & 0xff00000000000000ULL) >> 56) );
        }
    #endif

    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

        #define be16toh(__x)    ((uint16_t)(x))
        #define be32toh(__x)    ((uint32_t)(x))
        #define be64toh(__x)    ((uint64_t)(x))

    #elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

        #define be16toh(__x)    bswap16(__x)
        #define be32toh(__x)    bswap32(__x)
        #define be64toh(__x)    bswap64(__x)

    #endif

#endif /* platforms */

#define Swap16(x)    ( (x) = be16toh( (x) ) )
#define Swap32(x)    ( (x) = be32toh( (x) ) )
#define Swap64(x)    ( (x) = be64toh( (x) ) )

#if defined(__clang__)

#define Swap(x)  _Generic( (x), uint16_t:Swap16((x)),  uint32_t:Swap32((x)),  uint64_t:Swap64((x))  )

#else

#define Swap(x) { switch(sizeof((x))) { \
    case 16:Swap16((x)); break; \
    case 32:Swap32((x)); break; \
    case 64:Swap64((x)); break; \
    default: break; \
}}

#endif // defined(clang)

#endif // hfsinspect_endian_h

