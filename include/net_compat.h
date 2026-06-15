#ifndef NET_COMPAT_H
#define NET_COMPAT_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

typedef int socklen_t;
#define close closesocket
#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#ifndef EINTR
#define EINTR WSAEINTR
#endif
#define strcasecmp _stricmp
#define strncasecmp _strnicmp

static inline int net_compat_init(void) {
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa);
}

static inline void net_compat_cleanup(void) {
    WSACleanup();
}

#else
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

static inline int net_compat_init(void) {
    return 0;
}

static inline void net_compat_cleanup(void) {
}

#endif

#endif
