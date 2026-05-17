#ifndef DNS_PROTOCOL_H
#define DNS_PROTOCOL_H

#include <arpa/inet.h>
#include <stdint.h>

/* RFC 1035 DNS message header: 12 bytes */
typedef struct {
    uint16_t id;

    union {
        uint16_t value;
        struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
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
