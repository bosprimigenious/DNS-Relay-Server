# 平台相关头文件

| 文件 | 平台 | 作用 |
|------|------|------|
| `net_compat.h` | Linux + Windows | Socket / `select` / `inet_pton` 统一；`_WIN32` 走 Winsock，否则 POSIX |
| `endian.h` | Linux + Windows | DNS 报文头 bit-field 字节序宏 `DNS_LITTLE_ENDIAN_BITFIELD` |

业务代码请继续 `#include "net_compat.h"`（根目录入口会转发到本目录）。
