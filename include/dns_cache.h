#ifndef DNS_CACHE_H
#define DNS_CACHE_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "dns_protocol.h"

typedef struct {
    int valid;
    char qname[DNS_MAX_NAME_LEN + 1];
    uint16_t qtype;
    uint16_t qclass;
    unsigned char response[DNS_MAX_MESSAGE];
    int response_len;
    time_t stored_at;
    time_t expire_at;
} dns_cache_entry_t;

typedef struct {
    dns_cache_entry_t *entries;
    size_t capacity;
    size_t next_slot;
} dns_cache_t;

int dns_cache_init(dns_cache_t *cache, size_t capacity);

void dns_cache_destroy(dns_cache_t *cache);

void dns_cache_purge_expired(dns_cache_t *cache, time_t now);

int dns_cache_lookup(dns_cache_t *cache, const char *qname,
                     uint16_t qtype, uint16_t qclass, time_t now,
                     unsigned char *response_out, int response_out_size,
                     int *response_len_out, uint32_t *ttl_remaining_out);

int dns_cache_store(dns_cache_t *cache, const char *qname,
                    uint16_t qtype, uint16_t qclass,
                    const unsigned char *response, int response_len,
                    time_t now);

#endif
