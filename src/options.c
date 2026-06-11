#include "options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void options_copy_string(char *dst, size_t dst_size, const char *src) {
    if (dst_size == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    snprintf(dst, dst_size, "%s", src);
}

static int parse_port(const char *text, uint16_t *value_out) {
    char *endptr;
    unsigned long value;

    value = strtoul(text, &endptr, 10);
    if (text[0] == '\0' || *endptr != '\0' || value == 0 || value > 65535UL) {
        return -1;
    }

    *value_out = (uint16_t)value;
    return 0;
}

static int parse_size(const char *text, size_t *value_out) {
    char *endptr;
    unsigned long value;

    value = strtoul(text, &endptr, 10);
    if (text[0] == '\0' || *endptr != '\0' || value == 0UL) {
        return -1;
    }

    *value_out = (size_t)value;
    return 0;
}

void options_init(options_t *options) {
    const char *bind_env;
    const char *port_env;
    uint16_t port;

    options_copy_string(options->bind_ip, sizeof(options->bind_ip), "0.0.0.0");
    options->listen_port = 53;
    options_copy_string(options->upstream_ip, sizeof(options->upstream_ip),
                        "114.114.114.114");
    options_copy_string(options->hosts_file, sizeof(options->hosts_file),
                        DEFAULT_HOSTS_FILE);
    options->cache_size = 1024;
    options->verbosity = 0;
    options->show_help = 0;

    bind_env = getenv("DNS_RELAY_BIND");
    if (bind_env != NULL && bind_env[0] != '\0') {
        options_copy_string(options->bind_ip, sizeof(options->bind_ip), bind_env);
    }

    port_env = getenv("DNS_RELAY_PORT");
    if (port_env != NULL && port_env[0] != '\0' &&
        parse_port(port_env, &port) == 0) {
        options->listen_port = port;
    }
}

int options_parse(options_t *options, int argc, char **argv) {
    int i;

    options_init(options);

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            options->show_help = 1;
            return 0;
        }
        if (strcmp(argv[i], "-v") == 0) {
            if (options->verbosity < 2) {
                options->verbosity++;
            }
            continue;
        }
        if (strcmp(argv[i], "-vv") == 0) {
            options->verbosity = 2;
            continue;
        }
        if (strcmp(argv[i], "-b") == 0) {
            if (i + 1 >= argc) {
                return -1;
            }
            options_copy_string(options->bind_ip, sizeof(options->bind_ip),
                                argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 >= argc || parse_port(argv[i + 1], &options->listen_port) != 0) {
                return -1;
            }
            i++;
            continue;
        }
        if (strcmp(argv[i], "-s") == 0) {
            if (i + 1 >= argc) {
                return -1;
            }
            options_copy_string(options->upstream_ip, sizeof(options->upstream_ip),
                                argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 >= argc) {
                return -1;
            }
            options_copy_string(options->hosts_file, sizeof(options->hosts_file),
                                argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "-c") == 0) {
            if (i + 1 >= argc || parse_size(argv[i + 1], &options->cache_size) != 0) {
                return -1;
            }
            i++;
            continue;
        }

        return -1;
    }

    return 0;
}

void options_print_usage(const char *program, FILE *stream) {
    fprintf(stream,
            "Usage: %s [-b bind-ip] [-p listen-port] [-s upstream-ip]\n"
            "          [-f hosts-file] [-c cache-size] [-v|-vv]\n",
            program);
}
