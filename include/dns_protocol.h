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

#define DNS_PORT            53
#define DNS_MAX_MESSAGE     512
#define DNS_MAX_LABEL_LEN   63
#define DNS_MAX_NAME_LEN    255
#define DNS_QTYPE_A         1
#define DNS_QTYPE_AAAA      28
#define DNS_QTYPE_CNAME     5
#define DNS_QTYPE_MX        15
#define DNS_QTYPE_NS        2
#define DNS_QCLASS_IN       1
#define DNS_RCODE_NOERROR   0
#define DNS_RCODE_FORMAT    1
#define DNS_RCODE_SERVFAIL  2
#define DNS_RCODE_NXDOMAIN  3
#define DNS_RCODE_NOTIMP    4
#define DNS_RCODE_REFUSED   5
#define DNS_TYPE_A          1
#define DNS_TYPE_AAAA       28
#define DNS_TYPE_CNAME      5
#define DNS_TYPE_MX         15
#define DNS_TYPE_NS         2

typedef struct {
    uint16_t qtype;
    uint16_t qclass;
} dns_question_fixed_t;

typedef struct {
    uint16_t type;
    uint16_t class_;
    uint32_t ttl;
    uint16_t rdlength;
} dns_rr_fixed_t;

int dns_name_decode(const unsigned char *packet, int *offset,
                    char *out_buf, int buf_size);

int dns_name_encode(const char *name, unsigned char *out_buf, int buf_size);

int dns_name_skip(const unsigned char *packet, int *offset);

int dns_parse_query(const unsigned char *packet, int packet_len, char *qname,
                    int qname_size, uint16_t *qtype, uint16_t *qclass);

int dns_build_error_response(const unsigned char *query, int query_len,
                             unsigned char *response, int response_size,
                             uint8_t rcode);

int dns_build_a_response(const unsigned char *query, int query_len,
                         unsigned char *response, int response_size,
                         struct in_addr ip, uint32_t ttl);

#endif
