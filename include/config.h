#ifndef CONFIG_H
#define CONFIG_H

#include <netinet/in.h>

#define CONFIG_MAX_ENTRIES 4096

typedef struct {
    struct in_addr ip;
    char domain[256];
    int block_ipv6_only;    /* 1=只拦截IPv6(AAAA)，A记录照常走上游 */
} config_entry_t;

typedef struct {
    config_entry_t entries[CONFIG_MAX_ENTRIES];
    int count;
} config_t;

int config_load(const char *path, config_t *cfg);

const config_entry_t *config_lookup(const config_t *cfg, const char *domain);

#endif
