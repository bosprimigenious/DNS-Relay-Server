#include "id_map.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

static id_map_record_t g_records[ID_MAP_SIZE];
static int g_next_slot = 0;

static void clear_record_slot(id_map_record_t *record) {
    memset(record, 0, sizeof(*record));
}

int add_record(uint16_t original_id,
               uint16_t new_id,
               struct in_addr client_ip,
               uint16_t client_port,
               const char *qname,
               uint16_t qtype,
               uint16_t qclass,
               const unsigned char *query,
               int query_len,
               time_t created_at) {
    int checked = 0;

    if (query_len <= 0 || query_len > DNS_MAX_MESSAGE) {
        return -1;
    }

    while (checked < ID_MAP_SIZE && g_records[g_next_slot].in_use) {
        g_next_slot = (g_next_slot + 1) % ID_MAP_SIZE;
        checked++;
    }

    if (checked == ID_MAP_SIZE) {
        return -1;
    }

    g_records[g_next_slot].in_use = 1;
    g_records[g_next_slot].original_id = original_id;
    g_records[g_next_slot].new_id = new_id;
    g_records[g_next_slot].client_ip = client_ip;
    g_records[g_next_slot].client_port = client_port;
    g_records[g_next_slot].created_at = created_at;
    snprintf(g_records[g_next_slot].qname, sizeof(g_records[g_next_slot].qname),
             "%s", qname != NULL ? qname : "");
    g_records[g_next_slot].qtype = qtype;
    g_records[g_next_slot].qclass = qclass;
    g_records[g_next_slot].query_len = query_len;
    memcpy(g_records[g_next_slot].query, query, (size_t)query_len);

    g_next_slot = (g_next_slot + 1) % ID_MAP_SIZE;
    return 0;
}

id_map_record_t *find_record_by_new_id(uint16_t new_id) {
    int i;

    for (i = 0; i < ID_MAP_SIZE; i++) {
        if (g_records[i].in_use && g_records[i].new_id == new_id) {
            return &g_records[i];
        }
    }

    return NULL;
}

id_map_record_t *find_expired_record(time_t now, time_t timeout_seconds) {
    int i;

    for (i = 0; i < ID_MAP_SIZE; i++) {
        if (g_records[i].in_use &&
            now >= g_records[i].created_at &&
            (now - g_records[i].created_at) >= timeout_seconds) {
            return &g_records[i];
        }
    }

    return NULL;
}

void release_record(id_map_record_t *record) {
    if (record == NULL) {
        return;
    }
    clear_record_slot(record);
}

void clear_timeout_records(time_t now, time_t timeout_seconds) {
    int i;

    for (i = 0; i < ID_MAP_SIZE; i++) {
        if (g_records[i].in_use &&
            now >= g_records[i].created_at &&
            (now - g_records[i].created_at) >= timeout_seconds) {
            clear_record_slot(&g_records[i]);
        }
    }
}
