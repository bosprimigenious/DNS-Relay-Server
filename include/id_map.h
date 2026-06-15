#ifndef ID_MAP_H
#define ID_MAP_H

#include <stdint.h>
#include <time.h>
#include "net_compat.h"

#include "dns_protocol.h"

#define ID_MAP_SIZE 1024

typedef struct {
    int in_use;
    uint16_t original_id;
    uint16_t new_id;
    struct in_addr client_ip;
    uint16_t client_port;
    time_t created_at;
    char qname[DNS_MAX_NAME_LEN + 1];
    uint16_t qtype;
    uint16_t qclass;
    int query_len;
    unsigned char query[DNS_MAX_MESSAGE];
} id_map_record_t;

int add_record(uint16_t original_id,
               uint16_t new_id,
               struct in_addr client_ip,
               uint16_t client_port,
               const char *qname,
               uint16_t qtype,
               uint16_t qclass,
               const unsigned char *query,
               int query_len,
               time_t created_at);

id_map_record_t *find_record_by_new_id(uint16_t new_id);

id_map_record_t *find_expired_record(time_t now, time_t timeout_seconds);

void release_record(id_map_record_t *record);

void clear_timeout_records(time_t now, time_t timeout_seconds);

#endif
