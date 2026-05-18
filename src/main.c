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
#include "dns_protocol.h"
#include "id_map.h"

#define UPSTREAM_DNS_IP      "114.114.114.114"
#define ID_MAP_TIMEOUT_SEC   5
#define SELECT_TIMEOUT_USEC  10000

static config_t g_config;

static int relay_to_upstream(int sockfd,
                             const unsigned char *query,
                             int query_len,
                             uint16_t original_id,
                             struct sockaddr_in *client_addr);

static int relay_to_upstream(int sockfd,
                             const unsigned char *query,
                             int query_len,
                             uint16_t original_id,
                             struct sockaddr_in *client_addr) {
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
    if (inet_pton(AF_INET, UPSTREAM_DNS_IP, &upstream_addr.sin_addr) != 1) {
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

    upstream_len = sizeof(upstream_addr);
    resp_len = recvfrom(upstream_fd, response, sizeof(response), 0,
                        (struct sockaddr *)&upstream_addr, &upstream_len);
    if (resp_len < 0) {
        close(upstream_fd);
        return -1;
    }

    *(uint16_t *)response = htons(original_id);

    sent = sendto(sockfd, response, (size_t)resp_len, 0,
                  (const struct sockaddr *)client_addr, sizeof(*client_addr));
    close(upstream_fd);

    if (sent < 0) {
        return -1;
    }

    return 0;
}

int main(void) {
    int sockfd;
    struct sockaddr_in server_addr;

    int reuse = 1;
    const char *bind_ip;
    const char *port_env;
    int listen_port;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt SO_REUSEADDR");
        close(sockfd);
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bind_ip = getenv("DNS_RELAY_BIND");
    if (bind_ip != NULL && bind_ip[0] != '\0') {
        if (inet_pton(AF_INET, bind_ip, &server_addr.sin_addr) != 1) {
            fprintf(stderr, "invalid DNS_RELAY_BIND: %s\n", bind_ip);
            close(sockfd);
            return EXIT_FAILURE;
        }
    } else {
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    port_env = getenv("DNS_RELAY_PORT");
    listen_port = DNS_PORT;
    if (port_env != NULL && port_env[0] != '\0') {
        listen_port = atoi(port_env);
        if (listen_port <= 0 || listen_port > 65535) {
            fprintf(stderr, "invalid DNS_RELAY_PORT: %s\n", port_env);
            close(sockfd);
            return EXIT_FAILURE;
        }
    }
    server_addr.sin_port = htons((uint16_t)listen_port);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        if (errno == EACCES) {
            fprintf(stderr,
                    "bind: permission denied (port 53 usually requires elevated privileges)\n");
        }
        perror("bind");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (config_load("参考资料/dnsrelay.txt", &g_config) != 0) {
        fprintf(stderr, "warning: failed to load config, relay-only mode\n");
    } else {
        fprintf(stderr, "loaded %d config entries from 参考资料/dnsrelay.txt\n", g_config.count);
    }

    if (bind_ip != NULL && bind_ip[0] != '\0') {
        printf("DNS relay server listening on %s:%d ...\n", bind_ip, listen_port);
    } else {
        printf("DNS relay server listening on 0.0.0.0:%d ...\n", listen_port);
    }

    for (;;) {
        fd_set readfds;
        struct timeval timeout;
        int ready;

        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        timeout.tv_sec = 0;
        timeout.tv_usec = SELECT_TIMEOUT_USEC;

        ready = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            break;
        }

        if (ready == 0) {
            continue;
        }

        if (FD_ISSET(sockfd, &readfds)) {
            unsigned char buffer[DNS_MAX_MESSAGE];
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            ssize_t received;

            received = recvfrom(sockfd,
                                buffer,
                                sizeof(buffer),
                                0,
                                (struct sockaddr *)&client_addr,
                                &client_len);
            if (received < 0) {
                perror("recvfrom");
                continue;
            }

            if (received < 12) {
                continue;
            }

            clear_timeout_records(time(NULL), ID_MAP_TIMEOUT_SEC);

            {
                char qname[DNS_MAX_NAME_LEN + 1];
                uint16_t qtype;
                uint16_t qclass;
                const config_entry_t *entry;

                if (dns_parse_query(buffer, (int)received, qname, sizeof(qname),
                                    &qtype, &qclass) != 0) {
                    unsigned char err_resp[DNS_MAX_MESSAGE];
                    int err_len;

                    err_len = dns_build_error_response(buffer, (int)received, err_resp,
                                                       sizeof(err_resp), DNS_RCODE_FORMAT);
                    if (err_len > 0) {
                        sendto(sockfd, err_resp, (size_t)err_len, 0,
                               (const struct sockaddr *)&client_addr, client_len);
                    }
                    continue;
                }

                entry = config_lookup(&g_config, qname);
                if (entry != NULL) {
                    if (entry->ip.s_addr == 0) {
                        unsigned char resp[DNS_MAX_MESSAGE];
                        int rlen;

                        rlen = dns_build_error_response(buffer, (int)received, resp,
                                                        sizeof(resp), DNS_RCODE_NXDOMAIN);
                        if (rlen > 0) {
                            sendto(sockfd, resp, (size_t)rlen, 0,
                                   (const struct sockaddr *)&client_addr, client_len);
                        }
                    } else {
                        if (qtype == DNS_QTYPE_A) {
                            unsigned char resp[DNS_MAX_MESSAGE];
                            int rlen;

                            rlen = dns_build_a_response(buffer, (int)received, resp,
                                                        sizeof(resp), entry->ip, 300);
                            if (rlen > 0) {
                                sendto(sockfd, resp, (size_t)rlen, 0,
                                       (const struct sockaddr *)&client_addr, client_len);
                            }
                        } else {
                            unsigned char resp[DNS_MAX_MESSAGE];
                            int rlen;

                            rlen = dns_build_error_response(buffer, (int)received, resp,
                                                            sizeof(resp), DNS_RCODE_NOERROR);
                            if (rlen > 0) {
                                sendto(sockfd, resp, (size_t)rlen, 0,
                                       (const struct sockaddr *)&client_addr, client_len);
                            }
                        }
                    }
                } else {
                    const dns_header_t *hdr = (const dns_header_t *)buffer;
                    uint16_t original_id = ntohs(hdr->id);
                    int relay_ret;

                    relay_ret = relay_to_upstream(sockfd, buffer, (int)received,
                                                  original_id, &client_addr);
                    if (relay_ret != 0) {
                        unsigned char err_resp[DNS_MAX_MESSAGE];
                        int err_len;

                        err_len = dns_build_error_response(buffer, (int)received, err_resp,
                                                           sizeof(err_resp), DNS_RCODE_SERVFAIL);
                        if (err_len > 0) {
                            sendto(sockfd, err_resp, (size_t)err_len, 0,
                                   (const struct sockaddr *)&client_addr, client_len);
                        }
                    }
                }
            }
        }
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
