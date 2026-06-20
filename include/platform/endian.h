#ifndef PLATFORM_ENDIAN_H
#define PLATFORM_ENDIAN_H

#if defined(_WIN32) || defined(_MSC_VER)
#define DNS_LITTLE_ENDIAN_BITFIELD 1
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define DNS_LITTLE_ENDIAN_BITFIELD 1
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define DNS_LITTLE_ENDIAN_BITFIELD 0
#elif defined(BYTE_ORDER) && (BYTE_ORDER == LITTLE_ENDIAN)
#define DNS_LITTLE_ENDIAN_BITFIELD 1
#elif defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
#define DNS_LITTLE_ENDIAN_BITFIELD 0
#else
#error "Unable to determine host byte order for DNS bit-fields."
#endif

#endif
