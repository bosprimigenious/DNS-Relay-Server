#include "net_compat.h"

#include "config.h"
#include "dns_cache.h"
#include "dns_protocol.h"
#include "id_map.h"
#include "logger.h"
#include "options.h"
#include "relay_mode.h"

#define ID_MAP_TIMEOUT_SEC   5
#define SELECT_TIMEOUT_USEC  10000

static config_t g_config;
static dns_cache_t g_cache;
static uint16_t g_next_upstream_id = 1;

static uint16_t dns_read_id(const unsigned char *packet) {
    uint16_t net_id;

    memcpy(&net_id, packet, sizeof(net_id));
    return ntohs(net_id);
}

static void dns_write_id(unsigned char *packet, uint16_t id) {
    uint16_t net_id = htons(id);

    memcpy(packet, &net_id, sizeof(net_id));
}

static void make_client_addr(const id_map_record_t *record,
                             struct sockaddr_in *client_addr) {
    memset(client_addr, 0, sizeof(*client_addr));
    client_addr->sin_family = AF_INET;
    client_addr->sin_addr = record->client_ip;
    client_addr->sin_port = record->client_port;
}

static int send_packet(int sockfd, const unsigned char *packet, int packet_len,
                       const struct sockaddr_in *addr, socklen_t addr_len) {
    ssize_t sent;

    sent = sendto(sockfd, (const char *)packet, (size_t)packet_len, 0,
                  (const struct sockaddr *)addr, addr_len);
    if (sent < 0 || sent != packet_len) {
        return -1;
    }
    return 0;
}

static int send_error_response(int sockfd, const unsigned char *query, int query_len,
                               const struct sockaddr_in *client_addr,
                               socklen_t client_len, uint8_t rcode) {
    unsigned char response[DNS_MAX_MESSAGE];
    int response_len;

    response_len = dns_build_error_response(query, query_len, response,
                                            sizeof(response), rcode);
    if (response_len <= 0) {
        return -1;
    }
    return send_packet(sockfd, response, response_len, client_addr, client_len);
}

static uint16_t allocate_upstream_id(void) {
    uint16_t start = g_next_upstream_id;

    for (;;) {
        if (g_next_upstream_id == 0) {
            g_next_upstream_id = 1;
        }
        if (find_record_by_new_id(g_next_upstream_id) == NULL) {
            uint16_t allocated = g_next_upstream_id;

            g_next_upstream_id++;
            if (g_next_upstream_id == 0) {
                g_next_upstream_id = 1;
            }
            return allocated;
        }

        g_next_upstream_id++;
        if (g_next_upstream_id == 0) {
            g_next_upstream_id = 1;
        }
        if (g_next_upstream_id == start) {
            return 0;
        }
    }
}

static void process_expired_queries(int client_fd) {
    time_t now = time(NULL);
    id_map_record_t *record;

    while ((record = find_expired_record(now, ID_MAP_TIMEOUT_SEC)) != NULL) {
        struct sockaddr_in client_addr;

        make_client_addr(record, &client_addr);
        LOG_INFO("TIMEOUT", "qname=%s original_id=%u new_id=%u",
                 record->qname, record->original_id, record->new_id);
        if (send_error_response(client_fd, record->query, record->query_len,
                                &client_addr, sizeof(client_addr),
                                DNS_RCODE_SERVFAIL) != 0) {
            LOG_ERROR("ERROR", "failed to send timeout SERVFAIL for qname=%s",
                      record->qname);
        }
        release_record(record);
    }
}

static void handle_upstream_response(int client_fd, int upstream_fd,
                                     const struct sockaddr_in *upstream_addr) {
    unsigned char response[DNS_MAX_MESSAGE];
    struct sockaddr_in response_addr;
    socklen_t response_addr_len = sizeof(response_addr);
    id_map_record_t *record;
    time_t now;
    uint16_t new_id;
    ssize_t received;

    received = recvfrom(upstream_fd, (char *)response, sizeof(response), 0,
                        (struct sockaddr *)&response_addr, &response_addr_len);
    if (received < 0) {
#ifdef _WIN32
        if (WSAGetLastError() != WSAEINTR && WSAGetLastError() != WSAEWOULDBLOCK) {
            LOG_ERROR("ERROR", "recvfrom upstream failed: %d", WSAGetLastError());
        }
#else
        if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR("ERROR", "recvfrom upstream failed: %s", strerror(errno));
        }
#endif
        return;
    }

    if (response_addr.sin_addr.s_addr != upstream_addr->sin_addr.s_addr ||
        response_addr.sin_port != upstream_addr->sin_port) {
        LOG_INFO("UPSTREAM", "ignored packet from unexpected server");
        return;
    }
    if (received < 12) {
        LOG_INFO("UPSTREAM", "ignored short upstream response len=%zd", received);
        return;
    }

    new_id = dns_read_id(response);
    record = find_record_by_new_id(new_id);
    if (record == NULL) {
        LOG_INFO("UPSTREAM", "stale response new_id=%u", new_id);
        return;
    }

    now = time(NULL);
    if (dns_cache_store(&g_cache, record->qname, record->qtype, record->qclass,
                        response, (int)received, now) == 0) {
        LOG_DEBUG("UPSTREAM", "cached qname=%s", record->qname);
    }

    dns_write_id(response, record->original_id);

    {
        struct sockaddr_in client_addr;

        make_client_addr(record, &client_addr);
        if (send_packet(client_fd, response, (int)received,
                        &client_addr, sizeof(client_addr)) != 0) {
            LOG_ERROR("ERROR", "failed to send response for qname=%s", record->qname);
        } else {
            LOG_INFO("UPSTREAM", "qname=%s original_id=%u new_id=%u len=%zd",
                     record->qname, record->original_id, record->new_id, received);
        }
    }

    release_record(record);
}

static int try_local_response(int client_fd, const unsigned char *query, int query_len,
                              const struct sockaddr_in *client_addr,
                              socklen_t client_len, const char *qname,
                              uint16_t qtype, const config_entry_t *entry) {
    unsigned char response[DNS_MAX_MESSAGE];
    int response_len;

    if (entry->block_ipv6_only) {
        if (qtype == DNS_QTYPE_AAAA) {
            LOG_INFO("BLOCK", "qname=%s IPv6 blocked", qname);
            return send_error_response(client_fd, query, query_len, client_addr,
                                       client_len, DNS_RCODE_NXDOMAIN);
        }
        return 1;
    }

    if (entry->ip.s_addr == 0) {
        LOG_INFO("BLOCK", "qname=%s", qname);
        return send_error_response(client_fd, query, query_len, client_addr,
                                   client_len, DNS_RCODE_NXDOMAIN);
    }

    if (qtype == DNS_QTYPE_A) {
        response_len = dns_build_a_response(query, query_len, response,
                                            sizeof(response), entry->ip, 300);
        if (response_len > 0 &&
            send_packet(client_fd, response, response_len,
                        client_addr, client_len) == 0) {
            LOG_INFO("LOCAL", "qname=%s ip=%s", qname, inet_ntoa(entry->ip));
            return 0;
        }
        LOG_ERROR("ERROR", "failed to send local A response for qname=%s", qname);
        return -1;
    }

    LOG_INFO("LOCAL", "qname=%s qtype=%u empty NOERROR", qname, qtype);
    return send_error_response(client_fd, query, query_len, client_addr,
                               client_len, DNS_RCODE_NOERROR);
}

static void relay_query(int client_fd, int upstream_fd,
                        const struct sockaddr_in *upstream_addr,
                        unsigned char *query, int query_len,
                        const struct sockaddr_in *client_addr,
                        uint16_t original_id, const char *qname,
                        uint16_t qtype, uint16_t qclass, time_t now) {
    uint16_t new_id;
    int map_result;

    new_id = allocate_upstream_id();
    if (new_id == 0) {
        LOG_ERROR("ERROR", "no transaction id available for qname=%s", qname);
        send_error_response(client_fd, query, query_len, client_addr,
                            sizeof(*client_addr), DNS_RCODE_SERVFAIL);
        return;
    }

    map_result = add_record(original_id, new_id, client_addr->sin_addr,
                            client_addr->sin_port, qname, qtype, qclass,
                            query, query_len, now);
    if (map_result != 0) {
        LOG_ERROR("ERROR", "id map full for qname=%s", qname);
        send_error_response(client_fd, query, query_len, client_addr,
                            sizeof(*client_addr), DNS_RCODE_SERVFAIL);
        return;
    }

    dns_write_id(query, new_id);
    if (send_packet(upstream_fd, query, query_len,
                    upstream_addr, sizeof(*upstream_addr)) != 0) {
        id_map_record_t *record = find_record_by_new_id(new_id);

        if (record != NULL) {
            release_record(record);
        }
        dns_write_id(query, original_id);
        LOG_ERROR("ERROR", "failed to relay qname=%s", qname);
        send_error_response(client_fd, query, query_len, client_addr,
                            sizeof(*client_addr), DNS_RCODE_SERVFAIL);
        return;
    }

    LOG_INFO("RELAY", "qname=%s original_id=%u new_id=%u upstream=%s",
             qname, original_id, new_id, inet_ntoa(upstream_addr->sin_addr));
}

static int is_reverse_dns_qname(const char *qname) {
    size_t len;
    const char *suffix;

    if (qname == NULL) {
        return 0;
    }
    len = strlen(qname);
    suffix = ".in-addr.arpa";
    if (len >= strlen(suffix) &&
        strcasecmp(qname + len - strlen(suffix), suffix) == 0) {
        return 1;
    }
    suffix = ".ip6.arpa";
    if (len >= strlen(suffix) &&
        strcasecmp(qname + len - strlen(suffix), suffix) == 0) {
        return 1;
    }
    return 0;
}

static void handle_client_query(int client_fd, int upstream_fd,
                                const struct sockaddr_in *upstream_addr) {
    unsigned char query[DNS_MAX_MESSAGE];
    unsigned char response[DNS_MAX_MESSAGE];
    char qname[DNS_MAX_NAME_LEN + 1];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    const config_entry_t *entry;
    uint16_t original_id;
    uint16_t qtype;
    uint16_t qclass;
    ssize_t received;
    time_t now;
    int local_result;

    received = recvfrom(client_fd, (char *)query, sizeof(query), 0,
                        (struct sockaddr *)&client_addr, &client_len);
    if (received < 0) {
        return;
    }
    if (received < 12) {
        return;
    }

    original_id = dns_read_id(query);
    if (dns_parse_query(query, (int)received, qname, sizeof(qname),
                        &qtype, &qclass) != 0) {
        send_error_response(client_fd, query, (int)received, &client_addr,
                            client_len, DNS_RCODE_FORMAT);
        return;
    }

    if (qclass != DNS_QCLASS_IN) {
        send_error_response(client_fd, query, (int)received, &client_addr,
                            client_len, DNS_RCODE_NOTIMP);
        return;
    }

    if (is_reverse_dns_qname(qname)) {
        LOG_INFO("FAST", "qname=%s reverse-zone empty NOERROR", qname);
        send_error_response(client_fd, query, (int)received, &client_addr,
                            client_len, DNS_RCODE_NOERROR);
        return;
    }

    entry = config_lookup(&g_config, qname);
    if (entry != NULL) {
        local_result = try_local_response(client_fd, query, (int)received,
                                          &client_addr, client_len, qname,
                                          qtype, entry);
        if (local_result <= 0) {
            return;
        }
    }

    now = time(NULL);
    {
        int cached_len;
        uint32_t ttl_remaining = 0;
        int cache_hit;

        cache_hit = dns_cache_lookup(&g_cache, qname, qtype, qclass, now,
                                     response, sizeof(response),
                                     &cached_len, &ttl_remaining);
        if (cache_hit > 0) {
            dns_write_id(response, original_id);
            if (send_packet(client_fd, response, cached_len,
                            &client_addr, client_len) == 0) {
                LOG_INFO("CACHE", "qname=%s ttl=%u", qname, ttl_remaining);
            }
            return;
        }
    }

    if (qtype != DNS_QTYPE_A) {
        LOG_INFO("FAST", "qname=%s qtype=%u empty NOERROR (non-A, skip relay)", qname, qtype);
        send_error_response(client_fd, query, (int)received, &client_addr,
                            client_len, DNS_RCODE_NOERROR);
        return;
    }

    relay_query(client_fd, upstream_fd, upstream_addr, query, (int)received,
                &client_addr, original_id, qname, qtype, qclass, now);
}

int main(int argc, char **argv) {
    options_t options;
    struct sockaddr_in client_bind_addr;
    struct sockaddr_in upstream_addr;
    int client_fd;
    int upstream_fd;
    int reuse = 1;
    int maxfd;

    if (options_parse(&options, argc, argv) != 0) {
        options_print_usage(argv[0], stderr);
        return EXIT_FAILURE;
    }
    if (options.show_help) {
        options_print_usage(argv[0], stdout);
        return EXIT_SUCCESS;
    }

    if (net_compat_init() != 0) {
        fprintf(stderr, "network init failed\n");
        return EXIT_FAILURE;
    }

    logger_init(options.verbosity);

    memset(&client_bind_addr, 0, sizeof(client_bind_addr));
    client_bind_addr.sin_family = AF_INET;
    client_bind_addr.sin_port = htons(options.listen_port);
    if (inet_pton(AF_INET, options.bind_ip, &client_bind_addr.sin_addr) != 1) {
        LOG_ERROR("ERROR", "invalid bind IP: %s", options.bind_ip);
        net_compat_cleanup();
        return EXIT_FAILURE;
    }

    memset(&upstream_addr, 0, sizeof(upstream_addr));
    upstream_addr.sin_family = AF_INET;
    upstream_addr.sin_port = htons(DNS_PORT);
    if (inet_pton(AF_INET, options.upstream_ip, &upstream_addr.sin_addr) != 1) {
        LOG_ERROR("ERROR", "invalid upstream IP: %s", options.upstream_ip);
        net_compat_cleanup();
        return EXIT_FAILURE;
    }

    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0) {
        LOG_ERROR("ERROR", "socket client failed");
        net_compat_cleanup();
        return EXIT_FAILURE;
    }

    if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR,
                   (const char *)&reuse, sizeof(reuse)) < 0) {
        close(client_fd);
        net_compat_cleanup();
        return EXIT_FAILURE;
    }

    if (bind(client_fd, (const struct sockaddr *)&client_bind_addr,
             sizeof(client_bind_addr)) < 0) {
        LOG_ERROR("ERROR", "bind failed on %s:%u (need admin for port 53)",
                  options.bind_ip, options.listen_port);
        close(client_fd);
        net_compat_cleanup();
        return EXIT_FAILURE;
    }

    upstream_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (upstream_fd < 0) {
        close(client_fd);
        net_compat_cleanup();
        return EXIT_FAILURE;
    }

    if (config_load(options.hosts_file, &g_config) != 0) {
        LOG_INFO("INFO", "failed to load %s, relay-only mode", options.hosts_file);
    } else {
        LOG_INFO("INFO", "loaded %d entries from %s", g_config.count, options.hosts_file);
        if (options.verbosity >= 1) {
            config_print_policy_table(&g_config, stderr);
        }
    }

    if (dns_cache_init(&g_cache, options.cache_size) != 0) {
        close(upstream_fd);
        close(client_fd);
        net_compat_cleanup();
        return EXIT_FAILURE;
    }

    LOG_INFO("INFO", "relay mode: %s (%s)", DNS_RELAY_MODE_NAME, DNS_RELAY_MODE_LABEL);
    LOG_INFO("INFO", "build tag: %s", DNS_RELAY_BUILD_TAG);
    LOG_INFO("INFO", "listening on %s:%u upstream=%s cache=%zu",
             options.bind_ip, options.listen_port, options.upstream_ip,
             options.cache_size);
    fflush(stdout);

    maxfd = client_fd > upstream_fd ? client_fd : upstream_fd;

    for (;;) {
        fd_set readfds;
        struct timeval timeout;
        int ready;
        time_t now;

        now = time(NULL);
        process_expired_queries(client_fd);
        dns_cache_purge_expired(&g_cache, now);

        FD_ZERO(&readfds);
        FD_SET(client_fd, &readfds);
        FD_SET(upstream_fd, &readfds);

        timeout.tv_sec = 0;
        timeout.tv_usec = SELECT_TIMEOUT_USEC;

        ready = select(maxfd + 1, &readfds, NULL, NULL, &timeout);
        if (ready < 0) {
            continue;
        }
        if (ready == 0) {
            continue;
        }

        if (FD_ISSET(client_fd, &readfds)) {
            handle_client_query(client_fd, upstream_fd, &upstream_addr);
        }
        if (FD_ISSET(upstream_fd, &readfds)) {
            handle_upstream_response(client_fd, upstream_fd, &upstream_addr);
        }
    }

    dns_cache_destroy(&g_cache);
    close(upstream_fd);
    close(client_fd);
    net_compat_cleanup();
    return EXIT_FAILURE;
}
