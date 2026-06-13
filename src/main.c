/*
 * main.c — DNS 中继服务器主程序（同步上游中继 · relay-sync）
 *
 * 模块划分：
 *   1. 全局状态与常量
 *   2. 发送层 — UDP 发包与错误响应构造
 *   3. 同步上游中继 — 临时 socket、阻塞 recvfrom(3s)、还原 ID 回包
 *   4. 主程序 — 单 socket 初始化与 select 事件循环
 *   5. 查询调度（内联于主循环）— 拦截 / 本地解析 / 缓存 / 同步中继
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "dns_cache.h"
#include "dns_protocol.h"
#include "id_map.h"
#include "logger.h"
#include "options.h"
#include "relay_mode.h"

/* ---------- 模块 1：全局状态与常量 ---------- */

#define ID_MAP_TIMEOUT_SEC   5
#define SELECT_TIMEOUT_USEC  10000

static config_t g_config;
static dns_cache_t g_cache;

/* ---------- 模块 2：发送层 ---------- */

static int send_packet(int sockfd, const unsigned char *packet, int packet_len,
                       const struct sockaddr_in *addr, socklen_t addr_len) {
    ssize_t sent;

    sent = sendto(sockfd, packet, (size_t)packet_len, 0,
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

/* ---------- 模块 3：同步上游中继 ---------- */

static int relay_to_upstream(int sockfd,
                             const char *upstream_ip,
                             const unsigned char *query,
                             int query_len,
                             uint16_t original_id,
                             struct sockaddr_in *client_addr,
                             const char *qname,
                             uint16_t qtype,
                             uint16_t qclass) {
    int upstream_fd;
    struct sockaddr_in upstream_addr;
    struct timeval tv;
    static uint16_t g_next_id = 1;
    uint16_t new_id;
    unsigned char modified_query[DNS_MAX_MESSAGE];
    unsigned char response[DNS_MAX_MESSAGE];
    socklen_t upstream_len;
    ssize_t resp_len;
    ssize_t sent;
    time_t now;
    int map_ok;

    /* 每次中继临时创建上游 socket，用完即关闭 */
    upstream_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (upstream_fd < 0) {
        return -1;
    }

    tv.tv_sec = 3;
    tv.tv_usec = 0;
    if (setsockopt(upstream_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        close(upstream_fd);
        return -1;
    }

    memset(&upstream_addr, 0, sizeof(upstream_addr));
    upstream_addr.sin_family = AF_INET;
    upstream_addr.sin_port = htons(DNS_PORT);
    if (inet_pton(AF_INET, upstream_ip, &upstream_addr.sin_addr) != 1) {
        close(upstream_fd);
        return -1;
    }

    new_id = g_next_id++;
    if (g_next_id == 0) {
        g_next_id = 1;
    }

    now = time(NULL);
    map_ok = add_record(original_id, new_id, client_addr->sin_addr,
                        client_addr->sin_port, now);
    if (map_ok != 0) {
        clear_timeout_records(now, 0);
        map_ok = add_record(original_id, new_id, client_addr->sin_addr,
                            client_addr->sin_port, now);
        if (map_ok != 0) {
            close(upstream_fd);
            return -1;
        }
    }

    memcpy(modified_query, query, (size_t)query_len);
    *(uint16_t *)modified_query = htons(new_id);

    sent = sendto(upstream_fd, modified_query, (size_t)query_len, 0,
                  (const struct sockaddr *)&upstream_addr, sizeof(upstream_addr));
    if (sent < 0) {
        close(upstream_fd);
        return -1;
    }

    /* 阻塞等待上游响应，此期间主循环无法处理新查询 */
    upstream_len = sizeof(upstream_addr);
    resp_len = recvfrom(upstream_fd, response, sizeof(response), 0,
                        (struct sockaddr *)&upstream_addr, &upstream_len);
    if (resp_len < 0) {
        close(upstream_fd);
        return -1;
    }

    if (dns_cache_store(&g_cache, qname, qtype, qclass, response, (int)resp_len,
                        time(NULL)) == 0) {
        LOG_DEBUG("CACHE", "stored qname=%s", qname);
    }

    *(uint16_t *)response = htons(original_id);

    sent = sendto(sockfd, response, (size_t)resp_len, 0,
                  (const struct sockaddr *)client_addr, sizeof(*client_addr));
    close(upstream_fd);

    if (sent < 0) {
        return -1;
    }

    LOG_INFO("RELAY", "qname=%s upstream=%s len=%zd", qname, upstream_ip, resp_len);
    return 0;
}

/* ---------- 模块 4：主程序 — 初始化与事件循环 ---------- */

int main(int argc, char **argv) {
    options_t options;
    int sockfd;
    struct sockaddr_in server_addr;
    int reuse = 1;

    if (options_parse(&options, argc, argv) != 0) {
        options_print_usage(argv[0], stderr);
        return EXIT_FAILURE;
    }
    if (options.show_help) {
        options_print_usage(argv[0], stdout);
        return EXIT_SUCCESS;
    }

    logger_init(options.verbosity);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        LOG_ERROR("ERROR", "socket: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        LOG_ERROR("ERROR", "setsockopt SO_REUSEADDR: %s", strerror(errno));
        close(sockfd);
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, options.bind_ip, &server_addr.sin_addr) != 1) {
        LOG_ERROR("ERROR", "invalid bind IP: %s", options.bind_ip);
        close(sockfd);
        return EXIT_FAILURE;
    }
    server_addr.sin_port = htons(options.listen_port);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        if (errno == EACCES) {
            LOG_ERROR("ERROR", "bind permission denied (port %u may need root)",
                      options.listen_port);
        } else {
            LOG_ERROR("ERROR", "bind: %s", strerror(errno));
        }
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (config_load(options.hosts_file, &g_config) != 0) {
        LOG_INFO("INFO", "failed to load %s, relay-only mode", options.hosts_file);
    } else {
        LOG_INFO("INFO", "loaded %d entries from %s", g_config.count, options.hosts_file);
    }

    if (dns_cache_init(&g_cache, options.cache_size) != 0) {
        LOG_ERROR("ERROR", "cache init failed (size=%zu)", options.cache_size);
        close(sockfd);
        return EXIT_FAILURE;
    }

    LOG_INFO("INFO", "relay mode: %s (%s)", DNS_RELAY_MODE_NAME, DNS_RELAY_MODE_LABEL);
    LOG_INFO("INFO", "listening on %s:%u upstream=%s cache=%zu",
             options.bind_ip, options.listen_port, options.upstream_ip,
             options.cache_size);
    fflush(stdout);

    /* select 仅监听 client socket，10ms 超时避免忙等待 */
    for (;;) {
        fd_set readfds;
        struct timeval timeout;
        int ready;
        time_t now;

        now = time(NULL);
        clear_timeout_records(now, ID_MAP_TIMEOUT_SEC);
        dns_cache_purge_expired(&g_cache, now);

        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        timeout.tv_sec = 0;
        timeout.tv_usec = SELECT_TIMEOUT_USEC;

        ready = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            LOG_ERROR("ERROR", "select: %s", strerror(errno));
            break;
        }

        if (ready == 0) {
            continue;
        }

        /* ---------- 模块 5：查询调度（内联） ---------- */
        if (FD_ISSET(sockfd, &readfds)) {
            unsigned char buffer[DNS_MAX_MESSAGE];
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            ssize_t received;

            received = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                (struct sockaddr *)&client_addr, &client_len);
            if (received < 0) {
                LOG_ERROR("ERROR", "recvfrom: %s", strerror(errno));
                continue;
            }

            if (received < 12) {
                continue;
            }

            {
                char qname[DNS_MAX_NAME_LEN + 1];
                uint16_t qtype;
                uint16_t qclass;
                const config_entry_t *entry;
                const dns_header_t *hdr;
                uint16_t original_id;

                if (dns_parse_query(buffer, (int)received, qname, sizeof(qname),
                                    &qtype, &qclass) != 0) {
                    send_error_response(sockfd, buffer, (int)received, &client_addr,
                                      client_len, DNS_RCODE_FORMAT);
                    continue;
                }

                hdr = (const dns_header_t *)buffer;
                original_id = ntohs(hdr->id);

                if (qclass != DNS_QCLASS_IN) {
                    LOG_INFO("LOCAL", "unsupported qclass=%u qname=%s", qclass, qname);
                    send_error_response(sockfd, buffer, (int)received, &client_addr,
                                      client_len, DNS_RCODE_NOTIMP);
                    continue;
                }

                /* 5a. 本地拦截 / 本地解析（dnsrelay.txt 命中） */
                entry = config_lookup(&g_config, qname);
                if (entry != NULL) {
                    if (entry->ip.s_addr == 0) {
                        LOG_INFO("BLOCK", "qname=%s", qname);
                        send_error_response(sockfd, buffer, (int)received, &client_addr,
                                          client_len, DNS_RCODE_NXDOMAIN);
                    } else if (qtype == DNS_QTYPE_A) {
                        unsigned char resp[DNS_MAX_MESSAGE];
                        int rlen;

                        rlen = dns_build_a_response(buffer, (int)received, resp,
                                                    sizeof(resp), entry->ip, 300);
                        if (rlen > 0) {
                            send_packet(sockfd, resp, rlen, &client_addr, client_len);
                            LOG_INFO("LOCAL", "qname=%s ip=%s", qname,
                                     inet_ntoa(entry->ip));
                        }
                    } else {
                        send_error_response(sockfd, buffer, (int)received, &client_addr,
                                          client_len, DNS_RCODE_NOERROR);
                    }
                    continue;
                }

                /* 5b. TTL 缓存命中 */
                {
                    unsigned char cached_resp[DNS_MAX_MESSAGE];
                    int cached_len = 0;
                    uint32_t ttl_remaining = 0;
                    int cache_hit;

                    cache_hit = dns_cache_lookup(&g_cache, qname, qtype, qclass, now,
                                                 cached_resp, sizeof(cached_resp),
                                                 &cached_len, &ttl_remaining);
                    if (cache_hit > 0) {
                        *(uint16_t *)cached_resp = htons(original_id);
                        if (send_packet(sockfd, cached_resp, cached_len,
                                        &client_addr, client_len) == 0) {
                            LOG_INFO("CACHE", "qname=%s ttl=%u", qname, ttl_remaining);
                        }
                        continue;
                    }
                }

                /* 5c. 同步上游中继：函数内阻塞等待，失败回 SERVFAIL */
                if (relay_to_upstream(sockfd, options.upstream_ip, buffer,
                                      (int)received, original_id, &client_addr,
                                      qname, qtype, qclass) != 0) {
                    send_error_response(sockfd, buffer, (int)received, &client_addr,
                                      client_len, DNS_RCODE_SERVFAIL);
                }
            }
        }
    }

    dns_cache_destroy(&g_cache);
    close(sockfd);
    return EXIT_FAILURE;
}
