#include "id_map.h"

static id_map_record_t g_records[ID_MAP_SIZE];
static int g_next_slot = 0;

int add_record(uint16_t original_id,
               uint16_t new_id,
               struct in_addr client_ip,
               uint16_t client_port,
               time_t created_at) {
    int checked = 0;

    while (checked < ID_MAP_SIZE && g_records[g_next_slot].in_use) {
        g_next_slot = (g_next_slot + 1) % ID_MAP_SIZE;
        checked++;
    }

    if (checked == ID_MAP_SIZE && g_records[g_next_slot].in_use) {
        return -1;
    }

    g_records[g_next_slot].in_use = 1;
    g_records[g_next_slot].original_id = original_id;
    g_records[g_next_slot].new_id = new_id;
    g_records[g_next_slot].client_ip = client_ip;
    g_records[g_next_slot].client_port = client_port;
    g_records[g_next_slot].created_at = created_at;

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

    return 0;
}

void clear_timeout_records(time_t now, time_t timeout_seconds) {
    int i;

    for (i = 0; i < ID_MAP_SIZE; i++) {
        if (g_records[i].in_use && (now - g_records[i].created_at) > timeout_seconds) {
            g_records[i].in_use = 0;
        }
    }
}
