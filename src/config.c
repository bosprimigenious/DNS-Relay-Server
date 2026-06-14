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

static void config_print_row(const config_entry_t *entry, FILE *stream) {
    char ip_buf[INET_ADDRSTRLEN];

    if (entry->block_ipv6_only) {
        fprintf(stream, "| %-16s | ::              | %-10s | %-14s |\n",
                entry->domain, "RELAY", "NXDOMAIN");
        return;
    }

    if (entry->ip.s_addr == 0) {
        snprintf(ip_buf, sizeof(ip_buf), "0.0.0.0");
        fprintf(stream, "| %-16s | %-15s | %-10s | %-14s |\n",
                entry->domain, ip_buf, "NXDOMAIN", "NXDOMAIN");
        return;
    }

    inet_ntop(AF_INET, &entry->ip, ip_buf, sizeof(ip_buf));
    fprintf(stream, "| %-16s | %-15s | %-10s | %-14s |\n",
            entry->domain, ip_buf, "A record", "NOERROR(empty)");
}

void config_print_policy_table(const config_t *cfg, FILE *stream) {
    static const char *course_domains[] = {"bupt", "008.cn", "baidu.com"};
    int i;
    int j;
    int shown = 0;

    fprintf(stream, "\n");
    fprintf(stream, "+------------------+---------------+------------+----------------+\n");
    fprintf(stream, "| Domain           | IPv4 config   | A query    | AAAA query     |\n");
    fprintf(stream, "+------------------+---------------+------------+----------------+\n");
    fprintf(stream, "| baidu.com        | (not in file) | RELAY      | RELAY          |\n");

    for (i = 0; i < 3; i++) {
        const config_entry_t *entry;

        entry = config_lookup(cfg, course_domains[i]);
        if (entry != NULL) {
            config_print_row(entry, stream);
        }
    }

    for (i = 0; i < cfg->count; i++) {
        int skip;

        skip = 0;
        for (j = 0; j < 3; j++) {
            if (strcasecmp(cfg->entries[i].domain, course_domains[j]) == 0) {
                skip = 1;
                break;
            }
        }
        if (skip) {
            continue;
        }

        if (cfg->entries[i].block_ipv6_only ||
            cfg->entries[i].ip.s_addr == 0) {
            config_print_row(&cfg->entries[i], stream);
            shown++;
            if (shown >= 8) {
                break;
            }
        }
    }

    fprintf(stream, "+------------------+---------------+------------+----------------+\n");
    fprintf(stream, "| 0.0.0.0 = full block (A+AAAA NXDOMAIN)                       |\n");
    fprintf(stream, "| :: = IPv6-only block (A relay, AAAA NXDOMAIN)                 |\n");
    fprintf(stream, "| other IPv4 = local A; AAAA empty NOERROR (fix-A)             |\n");
    fprintf(stream, "| not in table = upstream relay via -s                         |\n");
    fprintf(stream, "+--------------------------------------------------------------+\n");
    fprintf(stream, "Total loaded: %d entries (table shows course cases + samples)\n\n",
            cfg->count);
}
