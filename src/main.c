#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "dns_protocol.h"
#include "id_map.h"

#define DNS_MAX_MESSAGE_SIZE 512
#define SELECT_TIMEOUT_USEC 10000

int main(void) {
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(53);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        if (errno == EACCES) {
            fprintf(stderr, "bind: permission denied (port 53 usually requires elevated privileges)\n");
        }
        perror("bind");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("DNS relay UDP server listening on port 53...\n");

    for (;;) {
        fd_set readfds;
        struct timeval timeout;
        int ready;

        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        timeout.tv_sec = 0;
        timeout.tv_usec = SELECT_TIMEOUT_USEC; /* 10 ms */

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
            unsigned char buffer[DNS_MAX_MESSAGE_SIZE];
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            char ip_str[INET_ADDRSTRLEN];
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

            if (inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str)) == NULL) {
                strncpy(ip_str, "unknown", sizeof(ip_str) - 1);
                ip_str[sizeof(ip_str) - 1] = '\0';
            }

            printf("Received %zd bytes from %s:%u\n",
                   received,
                   ip_str,
                   (unsigned int)ntohs(client_addr.sin_port));
        }
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
