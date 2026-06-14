#include "config.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

int config_load(const char *path, config_t *cfg) {
    FILE *fp;
    char line[512];
    char ip_str[64];
    char domain[256];

    cfg->count = 0;

    fp = fopen(path, "r");
    if (fp == NULL) {
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        config_entry_t *entry;

        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        if (sscanf(line, "%63s %255s", ip_str, domain) != 2) {
            continue;
        }

        entry = &cfg->entries[cfg->count];
        entry->block_ipv6_only = 0;   /* 默认不拦截IPv6 */

        if (strcmp(ip_str, "::") == 0) {
            entry->block_ipv6_only = 1;
            entry->ip.s_addr = 0;      /* 占位，不会走全拦截分支 */
        } else if (inet_pton(AF_INET, ip_str, &entry->ip) != 1) {
            fprintf(stderr, "config: skip invalid IP on line: %s", line);
            continue;
        }

        snprintf(entry->domain, sizeof(entry->domain), "%s", domain);
        cfg->count++;

        if (cfg->count >= CONFIG_MAX_ENTRIES) {
            break;
        }
    }

    fclose(fp);
    return 0;
}

const config_entry_t *config_lookup(const config_t *cfg, const char *domain) {
    int i;

    for (i = 0; i < cfg->count; i++) {
        if (strcasecmp(cfg->entries[i].domain, domain) == 0) {
            return &cfg->entries[i];
        }
    }

    return NULL;
}
