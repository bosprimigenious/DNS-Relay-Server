#ifndef ID_MAP_H
#define ID_MAP_H

#include <netinet/in.h>
#include <stdint.h>
#include <time.h>

#define ID_MAP_SIZE 1024

typedef struct {
    int in_use;
    uint16_t original_id;
    uint16_t new_id;
    struct in_addr client_ip;
    uint16_t client_port;
    time_t created_at;
} id_map_record_t;

int add_record(uint16_t original_id,
               uint16_t new_id,
               struct in_addr client_ip,
               uint16_t client_port,
               time_t created_at);

id_map_record_t *find_record_by_new_id(uint16_t new_id);

void clear_timeout_records(time_t now, time_t timeout_seconds);

#endif
