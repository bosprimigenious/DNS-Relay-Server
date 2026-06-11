#include "dns_cache.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static uint16_t dns_read_u16(const unsigned char *ptr) {
    uint16_t net_value;

    memcpy(&net_value, ptr, sizeof(net_value));
    return ntohs(net_value);
}

static uint32_t dns_read_u32(const unsigned char *ptr) {
    uint32_t net_value;

    memcpy(&net_value, ptr, sizeof(net_value));
    return ntohl(net_value);
}

static void dns_write_u32(unsigned char *ptr, uint32_t host_value) {
    uint32_t net_value = htonl(host_value);

    memcpy(ptr, &net_value, sizeof(net_value));
}

static void dns_cache_invalidate(dns_cache_entry_t *entry) {
    entry->valid = 0;
}

static int dns_skip_questions(const unsigned char *packet, int packet_len,
                              uint16_t qdcount, int *offset) {
    int i;

    for (i = 0; i < qdcount; i++) {
        if (dns_name_skip(packet, packet_len, offset) != 0) {
            return -1;
        }
        if (*offset + 4 > packet_len) {
            return -1;
        }
        *offset += 4;
    }

    return 0;
}

static int dns_step_rr(const unsigned char *packet, int packet_len, int *offset,
                       int *ttl_offset_out, uint32_t *ttl_out) {
    uint16_t rdlength;

    if (dns_name_skip(packet, packet_len, offset) != 0) {
        return -1;
    }
    if (*offset + 10 > packet_len) {
        return -1;
    }

    if (ttl_offset_out != NULL) {
        *ttl_offset_out = *offset + 4;
    }
    if (ttl_out != NULL) {
        *ttl_out = dns_read_u32(packet + *offset + 4);
    }

    rdlength = dns_read_u16(packet + *offset + 8);
    *offset += 10;
    if (*offset + rdlength > packet_len) {
        return -1;
    }
    *offset += rdlength;
    return 0;
}

static int dns_cache_min_answer_ttl(const unsigned char *response,
                                    int response_len, uint32_t *ttl_out) {
    const dns_header_t *header;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t rcode;
    uint32_t min_ttl = UINT_MAX;
    int offset = 12;
    int i;

    if (response_len < 12) {
        return -1;
    }

    header = (const dns_header_t *)response;
    qdcount = ntohs(header->qdcount);
    ancount = ntohs(header->ancount);
    rcode = ntohs(header->flags.value) & 0xF;

    if (rcode != DNS_RCODE_NOERROR || ancount == 0) {
        return -1;
    }
    if (dns_skip_questions(response, response_len, qdcount, &offset) != 0) {
        return -1;
    }

    for (i = 0; i < ancount; i++) {
        uint32_t ttl;

        if (dns_step_rr(response, response_len, &offset, NULL, &ttl) != 0) {
            return -1;
        }
        if (ttl < min_ttl) {
            min_ttl = ttl;
        }
    }

    if (min_ttl == UINT_MAX) {
        return -1;
    }

    *ttl_out = min_ttl;
    return 0;
}

static int dns_cache_adjust_ttls(unsigned char *response, int response_len,
                                 uint32_t elapsed) {
    const dns_header_t *header;
    uint16_t qdcount;
    uint16_t total_rrs;
    int offset = 12;
    int i;

    if (response_len < 12) {
        return -1;
    }

    header = (const dns_header_t *)response;
    qdcount = ntohs(header->qdcount);
    total_rrs = (uint16_t)(ntohs(header->ancount) + ntohs(header->nscount) +
                           ntohs(header->arcount));

    if (dns_skip_questions(response, response_len, qdcount, &offset) != 0) {
        return -1;
    }

    for (i = 0; i < total_rrs; i++) {
        int ttl_offset;
        uint32_t ttl;
        uint32_t remaining;

        if (dns_step_rr(response, response_len, &offset, &ttl_offset, &ttl) != 0) {
            return -1;
        }

        remaining = ttl > elapsed ? ttl - elapsed : 0;
        dns_write_u32(response + ttl_offset, remaining);
    }

    return 0;
}

int dns_cache_init(dns_cache_t *cache, size_t capacity) {
    if (capacity == 0) {
        return -1;
    }

    cache->entries = calloc(capacity, sizeof(*cache->entries));
    if (cache->entries == NULL) {
        return -1;
    }

    cache->capacity = capacity;
    cache->next_slot = 0;
    return 0;
}

void dns_cache_destroy(dns_cache_t *cache) {
    free(cache->entries);
    cache->entries = NULL;
    cache->capacity = 0;
    cache->next_slot = 0;
}

void dns_cache_purge_expired(dns_cache_t *cache, time_t now) {
    size_t i;

    for (i = 0; i < cache->capacity; i++) {
        if (cache->entries[i].valid && now >= cache->entries[i].expire_at) {
            dns_cache_invalidate(&cache->entries[i]);
        }
    }
}

int dns_cache_lookup(dns_cache_t *cache, const char *qname,
                     uint16_t qtype, uint16_t qclass, time_t now,
                     unsigned char *response_out, int response_out_size,
                     int *response_len_out, uint32_t *ttl_remaining_out) {
    size_t i;

    for (i = 0; i < cache->capacity; i++) {
        dns_cache_entry_t *entry = &cache->entries[i];
        uint32_t elapsed;
        uint32_t ttl_remaining;

        if (!entry->valid) {
            continue;
        }
        if (now >= entry->expire_at) {
            dns_cache_invalidate(entry);
            continue;
        }
        if (entry->qtype != qtype || entry->qclass != qclass ||
            strcasecmp(entry->qname, qname) != 0) {
            continue;
        }
        if (entry->response_len > response_out_size) {
            return -1;
        }

        memcpy(response_out, entry->response, (size_t)entry->response_len);
        elapsed = now > entry->stored_at ? (uint32_t)(now - entry->stored_at) : 0;
        if (dns_cache_adjust_ttls(response_out, entry->response_len, elapsed) != 0) {
            dns_cache_invalidate(entry);
            return 0;
        }

        ttl_remaining = (uint32_t)(entry->expire_at - now);
        *response_len_out = entry->response_len;
        if (ttl_remaining_out != NULL) {
            *ttl_remaining_out = ttl_remaining;
        }
        return 1;
    }

    return 0;
}

int dns_cache_store(dns_cache_t *cache, const char *qname,
                    uint16_t qtype, uint16_t qclass,
                    const unsigned char *response, int response_len,
                    time_t now) {
    dns_cache_entry_t *entry;
    uint32_t min_ttl;
    size_t i;
    size_t slot = cache->next_slot;

    if (response_len <= 0 || response_len > DNS_MAX_MESSAGE) {
        return -1;
    }
    if (dns_cache_min_answer_ttl(response, response_len, &min_ttl) != 0 ||
        min_ttl == 0) {
        return -1;
    }

    for (i = 0; i < cache->capacity; i++) {
        size_t candidate = (cache->next_slot + i) % cache->capacity;

        if (!cache->entries[candidate].valid ||
            now >= cache->entries[candidate].expire_at) {
            slot = candidate;
            break;
        }
    }

    entry = &cache->entries[slot];
    entry->valid = 1;
    snprintf(entry->qname, sizeof(entry->qname), "%s", qname);
    entry->qtype = qtype;
    entry->qclass = qclass;
    memcpy(entry->response, response, (size_t)response_len);
    entry->response_len = response_len;
    entry->stored_at = now;
    entry->expire_at = now + (time_t)min_ttl;

    cache->next_slot = (slot + 1) % cache->capacity;
    return 0;
}
