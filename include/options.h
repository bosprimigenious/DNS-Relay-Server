#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#define OPTIONS_MAX_IP 64
#define OPTIONS_MAX_PATH 512
#define DEFAULT_HOSTS_FILE "参考资料/dnsrelay.txt"

typedef struct {
    char bind_ip[OPTIONS_MAX_IP];
    uint16_t listen_port;
    char upstream_ip[OPTIONS_MAX_IP];
    char hosts_file[OPTIONS_MAX_PATH];
    size_t cache_size;
    int verbosity;
    int show_help;
} options_t;

void options_init(options_t *options);

int options_parse(options_t *options, int argc, char **argv);

void options_print_usage(const char *program, FILE *stream);

#endif
