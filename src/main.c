/* main.c — 同步上游中继（relay-sync）· 查询分流：拦截/本地/缓存/同步中继 */

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

#define ID_MAP_TIMEOUT_SEC   5      /* ID 映射表超时秒数，超时静默释放 */
#define SELECT_TIMEOUT_USEC  10000  /* select 10ms 超时，避免 CPU 忙等待 */

static config_t g_config;   /* dnsrelay.txt 本地域名表 */
static dns_cache_t g_cache; /* 上游响应 TTL 缓存 */

/* 函数：send_packet — UDP 单包发送，长度不完整则失败 */
static int send_packet(int sockfd, const unsigned char *packet, int packet_len,
                       const struct sockaddr_in *addr, socklen_t addr_len) {
    ssize_t sent;

    sent = sendto(sockfd, packet, (size_t)packet_len, 0,
                  (const struct sockaddr *)addr, addr_len);
    if (sent < 0 || sent != packet_len) { /* sendto 失败或只发出部分字节 */
        return -1;
    }
    return 0;
}

/* 函数：send_error_response — 按原查询构造错误响应（设 QR=1 + RCODE）后回包 */
static int send_error_response(int sockfd, const unsigned char *query, int query_len,
                               const struct sockaddr_in *client_addr,
                               socklen_t client_len, uint8_t rcode) {
    unsigned char response[DNS_MAX_MESSAGE];
    int response_len;

    response_len = dns_build_error_response(query, query_len, response,
                                            sizeof(response), rcode);
    if (response_len <= 0) { /* 构造失败（查询格式非法等） */
        return -1;
    }
    return send_packet(sockfd, response, response_len, client_addr, client_len);
}

/* 函数：relay_to_upstream — 同步转发：sendto 上游后阻塞 recvfrom(3s) 再回客户端 */
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

    upstream_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (upstream_fd < 0) { /* 临时上游 socket 创建失败 */
        return -1;
    }

    tv.tv_sec = 3;
    tv.tv_usec = 0;
    if (setsockopt(upstream_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        /* 设置接收超时 3 秒，超时后 recvfrom 返回 -1 */
        close(upstream_fd);
        return -1;
    }

    memset(&upstream_addr, 0, sizeof(upstream_addr));
    upstream_addr.sin_family = AF_INET;
    upstream_addr.sin_port = htons(DNS_PORT);
    if (inet_pton(AF_INET, upstream_ip, &upstream_addr.sin_addr) != 1) {
        /* 上游 IP 格式非法 */
        close(upstream_fd);
        return -1;
    }

    new_id = g_next_id++;
    if (g_next_id == 0) { /* ID 递增绕回 1（0 保留） */
        g_next_id = 1;
    }

    now = time(NULL);
    map_ok = add_record(original_id, new_id, client_addr->sin_addr,
                        client_addr->sin_port, now);
    if (map_ok != 0) { /* 映射表满，先强制清理再重试一次 */
        clear_timeout_records(now, 0);
        map_ok = add_record(original_id, new_id, client_addr->sin_addr,
                            client_addr->sin_port, now);
        if (map_ok != 0) { /* 仍满则放弃 */
            close(upstream_fd);
            return -1;
        }
    }

    memcpy(modified_query, query, (size_t)query_len);
    *(uint16_t *)modified_query = htons(new_id); /* 发给上游前替换 Transaction ID */

    sent = sendto(upstream_fd, modified_query, (size_t)query_len, 0,
                  (const struct sockaddr *)&upstream_addr, sizeof(upstream_addr));
    if (sent < 0) { /* 转发上游失败 */
        close(upstream_fd);
        return -1;
    }

    upstream_len = sizeof(upstream_addr);
    resp_len = recvfrom(upstream_fd, response, sizeof(response), 0,
                        (struct sockaddr *)&upstream_addr, &upstream_len);
    if (resp_len < 0) {
        /* 阻塞等待上游响应失败（含 3s 超时）← 此期间主循环无法收新查询 */
        close(upstream_fd);
        return -1;
    }

    if (dns_cache_store(&g_cache, qname, qtype, qclass, response, (int)resp_len,
                        time(NULL)) == 0) { /* 写入 TTL 缓存成功 */
        LOG_DEBUG("CACHE", "stored qname=%s", qname);
    }

    *(uint16_t *)response = htons(original_id); /* 回客户端前还原原始 Transaction ID */

    sent = sendto(sockfd, response, (size_t)resp_len, 0,
                  (const struct sockaddr *)client_addr, sizeof(*client_addr));
    close(upstream_fd); /* 每次中继用完即关闭临时 socket */

    if (sent < 0) { /* 回客户端失败 */
        return -1;
    }

    LOG_INFO("RELAY", "qname=%s upstream=%s len=%zd", qname, upstream_ip, resp_len);
    return 0;
}

/* 函数：main — 初始化单 socket，select 单路事件循环，查询调度内联处理 */
int main(int argc, char **argv) {
    options_t options;
    int sockfd;
    struct sockaddr_in server_addr;
    int reuse = 1;

    if (options_parse(&options, argc, argv) != 0) { /* CLI 参数非法 */
        options_print_usage(argv[0], stderr);
        return EXIT_FAILURE;
    }
    if (options.show_help) { /* -h 显示帮助后退出 */
        options_print_usage(argv[0], stdout);
        return EXIT_SUCCESS;
    }

    logger_init(options.verbosity);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { /* 创建监听 socket 失败 */
        LOG_ERROR("ERROR", "socket: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        /* 允许端口快速复用 */
        LOG_ERROR("ERROR", "setsockopt SO_REUSEADDR: %s", strerror(errno));
        close(sockfd);
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, options.bind_ip, &server_addr.sin_addr) != 1) {
        /* 监听 IP 格式非法 */
        LOG_ERROR("ERROR", "invalid bind IP: %s", options.bind_ip);
        close(sockfd);
        return EXIT_FAILURE;
    }
    server_addr.sin_port = htons(options.listen_port);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        if (errno == EACCES) {
            /* 低端口（如 53）无 root 权限 */
            LOG_ERROR("ERROR", "bind permission denied (port %u may need root)",
                      options.listen_port);
        } else { /* 端口被占用等其他 bind 错误 */
            LOG_ERROR("ERROR", "bind: %s", strerror(errno));
        }
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (config_load(options.hosts_file, &g_config) != 0) {
        /* 配置文件缺失：仅中继模式，不拦截/本地解析 */
        LOG_INFO("INFO", "failed to load %s, relay-only mode", options.hosts_file);
    } else {
        LOG_INFO("INFO", "loaded %d entries from %s", g_config.count, options.hosts_file);
    }

    if (dns_cache_init(&g_cache, options.cache_size) != 0) { /* 缓存初始化失败 */
        LOG_ERROR("ERROR", "cache init failed (size=%zu)", options.cache_size);
        close(sockfd);
        return EXIT_FAILURE;
    }

    LOG_INFO("INFO", "relay mode: %s (%s)", DNS_RELAY_MODE_NAME, DNS_RELAY_MODE_LABEL);
    LOG_INFO("INFO", "listening on %s:%u upstream=%s cache=%zu",
             options.bind_ip, options.listen_port, options.upstream_ip,
             options.cache_size);
    fflush(stdout);

    for (;;) { /* 事件循环：每轮先清理超时，再 select 监听客户端 */
        fd_set readfds;
        struct timeval timeout;
        int ready;
        time_t now;

        now = time(NULL);
        clear_timeout_records(now, ID_MAP_TIMEOUT_SEC); /* 静默清理过期 ID 映射 */
        dns_cache_purge_expired(&g_cache, now);         /* 清理过期缓存条目 */

        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds); /* 仅监听客户端 socket（不同步监听上游） */

        timeout.tv_sec = 0;
        timeout.tv_usec = SELECT_TIMEOUT_USEC;

        ready = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (ready < 0) { /* select 出错 */
            if (errno == EINTR) { /* 被信号中断，重新 select */
                continue;
            }
            LOG_ERROR("ERROR", "select: %s", strerror(errno));
            break;
        }

        if (ready == 0) { /* 10ms 内无 socket 可读，进入下一轮 */
            continue;
        }

        if (FD_ISSET(sockfd, &readfds)) { /* 客户端有查询 → 内联分流处理 */
            unsigned char buffer[DNS_MAX_MESSAGE];
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            ssize_t received;

            received = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                (struct sockaddr *)&client_addr, &client_len);
            if (received < 0) { /* 读客户端失败，跳过本轮 */
                LOG_ERROR("ERROR", "recvfrom: %s", strerror(errno));
                continue;
            }

            if (received < 12) { /* 不足 DNS 首部 12 字节，丢弃 */
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
                    /* 域名/问题段解析失败 → FORMERR */
                    send_error_response(sockfd, buffer, (int)received, &client_addr,
                                      client_len, DNS_RCODE_FORMAT);
                    continue;
                }

                hdr = (const dns_header_t *)buffer;
                original_id = ntohs(hdr->id);

                if (qclass != DNS_QCLASS_IN) { /* 仅支持 Internet 类查询 → NOTIMP */
                    LOG_INFO("LOCAL", "unsupported qclass=%u qname=%s", qclass, qname);
                    send_error_response(sockfd, buffer, (int)received, &client_addr,
                                      client_len, DNS_RCODE_NOTIMP);
                    continue;
                }

                entry = config_lookup(&g_config, qname); /* 查 dnsrelay.txt 本地表 */
                if (entry != NULL) { /* 命中本地配置 → 拦截或本地解析 */
                    if (entry->ip.s_addr == 0) {
                        /* IP=0.0.0.0 → 本地拦截，回 NXDOMAIN */
                        LOG_INFO("BLOCK", "qname=%s", qname);
                        send_error_response(sockfd, buffer, (int)received, &client_addr,
                                          client_len, DNS_RCODE_NXDOMAIN);
                    } else if (qtype == DNS_QTYPE_A) {
                        /* 本地解析：用配置 IP 构造 A 记录，TTL=300 */
                        unsigned char resp[DNS_MAX_MESSAGE];
                        int rlen;

                        rlen = dns_build_a_response(buffer, (int)received, resp,
                                                    sizeof(resp), entry->ip, 300);
                        if (rlen > 0) { /* 构造成功才发送 */
                            send_packet(sockfd, resp, rlen, &client_addr, client_len);
                            LOG_INFO("LOCAL", "qname=%s ip=%s", qname,
                                     inet_ntoa(entry->ip));
                        }
                    } else {
                        /* 本地有记录但查询类型非 A → 空 NOERROR（无 ANSWER 段） */
                        send_error_response(sockfd, buffer, (int)received, &client_addr,
                                          client_len, DNS_RCODE_NOERROR);
                    }
                    continue;
                }

                {
                    unsigned char cached_resp[DNS_MAX_MESSAGE];
                    int cached_len = 0;
                    uint32_t ttl_remaining = 0;
                    int cache_hit;

                    cache_hit = dns_cache_lookup(&g_cache, qname, qtype, qclass, now,
                                                 cached_resp, sizeof(cached_resp),
                                                 &cached_len, &ttl_remaining);
                    if (cache_hit > 0) { /* 缓存命中 → 替换 ID 后直接回包 */
                        *(uint16_t *)cached_resp = htons(original_id);
                        if (send_packet(sockfd, cached_resp, cached_len,
                                        &client_addr, client_len) == 0) {
                            LOG_INFO("CACHE", "qname=%s ttl=%u", qname, ttl_remaining);
                        }
                        continue;
                    }
                    /* cache_hit <= 0：未命中，继续走上游中继 */
                }

                if (relay_to_upstream(sockfd, options.upstream_ip, buffer,
                                      (int)received, original_id, &client_addr,
                                      qname, qtype, qclass) != 0) {
                    /* 同步中继失败（超时/网络错误等）→ 回 SERVFAIL */
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
