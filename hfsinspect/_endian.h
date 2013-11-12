//
//  _endian.h
//  hfsinspect
//
//  Created by Adam Knight on 11/10/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_endian_h
#define hfsinspect_endian_h

static inline uint16_t _bswap16(uint16_t x)
{
    uint16_t __x = (x);
    return ((uint16_t)(
                       (((uint16_t)(__x) & (uint16_t)0x00ffU) << 8) |
                       (((uint16_t)(__x) & (uint16_t)0xff00U) >> 8) ));
}

static inline uint32_t _bswap32(uint32_t x)
{
    uint32_t __x = (x);
    return ((uint32_t)(
                       (((uint32_t)(__x) & (uint32_t)0x000000ffUL) << 24) |
                       (((uint32_t)(__x) & (uint32_t)0x0000ff00UL) <<  8) |
                       (((uint32_t)(__x) & (uint32_t)0x00ff0000UL) >>  8) |
                       (((uint32_t)(__x) & (uint32_t)0xff000000UL) >> 24) ));
}

static inline uint64_t _bswap64(uint64_t x)
{
    uint64_t __x = (x);
    return ((uint64_t)(
                       (uint64_t)(((uint64_t)(__x) & (uint64_t)0x00000000000000ffULL) << 56) |
                       (uint64_t)(((uint64_t)(__x) & (uint64_t)0x000000000000ff00ULL) << 40) |
                       (uint64_t)(((uint64_t)(__x) & (uint64_t)0x0000000000ff0000ULL) << 24) |
                       (uint64_t)(((uint64_t)(__x) & (uint64_t)0x00000000ff000000ULL) <<  8) |
                       (uint64_t)(((uint64_t)(__x) & (uint64_t)0x000000ff00000000ULL) >>  8) |
                       (uint64_t)(((uint64_t)(__x) & (uint64_t)0x0000ff0000000000ULL) >> 24) |
                       (uint64_t)(((uint64_t)(__x) & (uint64_t)0x00ff000000000000ULL) >> 40) |
                       (uint64_t)(((uint64_t)(__x) & (uint64_t)0xff00000000000000ULL) >> 56) ));
}

#define _noswap16(x) ((uint16_t)(x))
#define _noswap32(x) ((uint32_t)(x))
#define _noswap64(x) ((uint64_t)(x))

// ... and now the platform and CPU-specific macrofest.

#if __APPLE__

    // Apple takes care of swapping out native functions for us.
    #include <libkern/OSByteOrder.h>

    #define htobe16(__x)   OSSwapHostToBigInt16(__x)
    #define htobe32(__x)   OSSwapHostToBigInt32(__x)
    #define htobe64(__x)   OSSwapHostToBigInt64(__x)

    #define be16toh(__x)   OSSwapBigToHostInt16(__x)
    #define be32toh(__x)   OSSwapBigToHostInt32(__x)
    #define be64toh(__x)   OSSwapBigToHostInt64(__x)

    #define bswap16(__x)   OSSwapInt16(__x)
    #define bswap32(__x)   OSSwapInt32(__x)
    #define bswap64(__x)   OSSwapInt64(__x)

#elif __FREEBSD__ || __NETBSD__

    // These are all native functions on the BSDs.
    #include <sys/endian.h>

#else /* other platforms */

    #define bswap16(x) _bswap16(x)
    #define bswap32(x) _bswap32(x)
    #define bswap64(x) _bswap64(x)

    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

        #define htobe16(x)      _noswap16(x)
        #define htobe32(x)      _noswap32(x)
        #define htobe64(x)      _noswap64(x)

        #define be16toh(__x)    _noswap16(__x)
        #define be32toh(__x)    _noswap32(__x)
        #define be64toh(__x)    _noswap64(__x)

        #define htole16(__x)    bswap16(__x)
        #define htole32(__x)    bswap32(__x)
        #define htole64(__x)    bswap64(__x)

        #define le16toh(__x)    bswap16(__x)
        #define le32toh(__x)    bswap32(__x)
        #define le64toh(__x)    bswap64(__x)

    #elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

        #define htobe16(__x)    bswap16(__x)
        #define htobe32(__x)    bswap32(__x)
        #define htobe64(__x)    bswap64(__x)

        #define be16toh(__x)    bswap16(__x)
        #define be32toh(__x)    bswap32(__x)
        #define be64toh(__x)    bswap64(__x)

        #define htole16(x)      _noswap16(x)
        #define htole32(x)      _noswap32(x)
        #define htole64(x)      _noswap64(x)

        #define le16toh(__x)    _noswap16(x)
        #define le32toh(__x)    _noswap32(x)
        #define le64toh(__x)    _noswap64(x)

    #endif

#endif /* platforms */

#define Convert16(x)    ( (x) = be16toh( (x) ) )
#define Convert32(x)    ( (x) = be32toh( (x) ) )
#define Convert64(x)    ( (x) = be64toh( (x) ) )

#endif

