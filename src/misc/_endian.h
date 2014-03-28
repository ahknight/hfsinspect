//
//  _endian.h
//  hfsinspect
//
//  Created by Adam Knight on 11/10/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_endian_h
#define hfsinspect_endian_h

#define _noswap16(x) ((uint16_t)(x))
#define _noswap32(x) ((uint32_t)(x))
#define _noswap64(x) ((uint64_t)(x))

// ... and now the platform and CPU-specific macrofest.

#if defined(__APPLE__)

    // Apple takes care of swapping out native functions for us.
    #include <libkern/OSByteOrder.h>

    #define be16toh(__x)   OSSwapBigToHostInt16(__x)
    #define be32toh(__x)   OSSwapBigToHostInt32(__x)
    #define be64toh(__x)   OSSwapBigToHostInt64(__x)

//    #define bswap16(__x)   OSSwapInt16(__x)
//    #define bswap32(__x)   OSSwapInt32(__x)
//    #define bswap64(__x)   OSSwapInt64(__x)

#elif defined(__FreeBSD__) || defined(__NetBSD__)

    #include <sys/endian.h>

#elif defined(__linux__)

    #include <endian.h>

//    #define bswap16(__x)   __bswap_16(__x)
//    #define bswap32(__x)   __bswap_32(__x)
//    #define bswap64(__x)   __bswap_64(__x)

#else /* other platforms */

#if defined __clang__ || defined __GNUC__ && (__GNUC__ > 4 || (__xGNUC__ == 4 && __GNUC_PATCHLEVEL__ > 8 ))
static inline uint16_t _bswap16 (uint16_t x) { return __builtin_bswap16(x); }
#else
static inline uint16_t _bswap16 (uint16_t x) { return (uint16_t)(((x & 0x00ff) << 8)|((x & 0xff00) >> 8)); }
#endif

#if defined __clang__ || defined __GNUC__ && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_PATCHLEVEL__ > 3 ))
static inline uint32_t _bswap32 (uint32_t x) { return __builtin_bswap32(x); }
static inline uint64_t _bswap64 (uint64_t x) { return __builtin_bswap64(x); }
#else
#warning Using fallback swapping functions.
static inline uint32_t _bswap32(uint32_t x)
{
    return (uint32_t)(
                      ((x & 0x000000ffU) << 24) |
                      ((x & 0x0000ff00U) <<  8) |
                      ((x & 0x00ff0000U) >>  8) |
                      ((x & 0xff000000U) >> 24) );
}

static inline uint64_t _bswap64(uint64_t x)
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

    #define bswap16(x) _bswap16(x)
    #define bswap32(x) _bswap32(x)
    #define bswap64(x) _bswap64(x)

    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        #define be16toh(__x)    _noswap16(__x)
        #define be32toh(__x)    _noswap32(__x)
        #define be64toh(__x)    _noswap64(__x)
    #elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define be16toh(__x)    bswap16(__x)
        #define be32toh(__x)    bswap32(__x)
        #define be64toh(__x)    bswap64(__x)
    #endif

#endif /* platforms */

#define Swap16(x)    ( (x) = be16toh( (x) ) )
#define Swap32(x)    ( (x) = be32toh( (x) ) )
#define Swap64(x)    ( (x) = be64toh( (x) ) )

#ifdef __clang__
#define Swap(x)  _Generic( (x), uint16_t:Swap16((x)),  uint32_t:Swap32((x)),  uint64_t:Swap64((x))  )
#else
#define Swap(x) { switch(sizeof((x))) { \
    case 16:Swap16((x)); break; \
    case 32:Swap32((x)); break; \
    case 64:Swap64((x)); break; \
    default: break; \
}}
#endif

#endif

