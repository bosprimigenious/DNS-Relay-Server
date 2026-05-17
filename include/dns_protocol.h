#ifndef DNS_PROTOCOL_H
#define DNS_PROTOCOL_H

#include <arpa/inet.h>
#include <stdint.h>
#if defined(__linux__) || defined(__GLIBC__)
#include <endian.h>
#endif

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define DNS_LITTLE_ENDIAN_BITFIELD 1
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define DNS_LITTLE_ENDIAN_BITFIELD 0
#elif defined(BYTE_ORDER) && (BYTE_ORDER == LITTLE_ENDIAN)
#define DNS_LITTLE_ENDIAN_BITFIELD 1
#elif defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
#define DNS_LITTLE_ENDIAN_BITFIELD 0
#else
#error "Unable to determine host byte order for DNS bit-fields. Please define __BYTE_ORDER__ or BYTE_ORDER for your platform."
#endif

/* RFC 1035 DNS message header: 12 bytes */
typedef struct {
    uint16_t id;

    union {
        uint16_t value;
        struct {
#if DNS_LITTLE_ENDIAN_BITFIELD
            uint16_t rcode : 4;
            uint16_t z : 3;
            uint16_t ra : 1;
            uint16_t rd : 1;
            uint16_t tc : 1;
            uint16_t aa : 1;
            uint16_t opcode : 4;
            uint16_t qr : 1;
#else
            uint16_t qr : 1;
            uint16_t opcode : 4;
            uint16_t aa : 1;
            uint16_t tc : 1;
            uint16_t rd : 1;
            uint16_t ra : 1;
            uint16_t z : 3;
            uint16_t rcode : 4;
#endif
        } bits;
    } flags;

    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} dns_header_t;

static inline uint16_t dns_flags_host_to_network(uint16_t host_flags) {
    return htons(host_flags);
}

static inline uint16_t dns_flags_network_to_host(uint16_t net_flags) {
    return ntohs(net_flags);
}

static inline void dns_header_host_to_network(dns_header_t *header) {
    header->id = htons(header->id);
    header->flags.value = dns_flags_host_to_network(header->flags.value);
    header->qdcount = htons(header->qdcount);
    header->ancount = htons(header->ancount);
    header->nscount = htons(header->nscount);
    header->arcount = htons(header->arcount);
}

static inline void dns_header_network_to_host(dns_header_t *header) {
    header->id = ntohs(header->id);
    header->flags.value = dns_flags_network_to_host(header->flags.value);
    header->qdcount = ntohs(header->qdcount);
    header->ancount = ntohs(header->ancount);
    header->nscount = ntohs(header->nscount);
    header->arcount = ntohs(header->arcount);
}

#endif
