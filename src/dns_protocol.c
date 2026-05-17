#include "dns_protocol.h"

#include <string.h>

int dns_name_skip(const unsigned char *packet, int *offset) {
    int label_len;

    for (;;) {
        label_len = packet[*offset];
        if (label_len == 0) {
            *offset += 1;
            return 0;
        }
        if ((label_len & 0xC0) == 0xC0) {
            *offset += 2;
            return 0;
        }
        *offset += 1 + label_len;
        if (*offset > DNS_MAX_MESSAGE) {
            return -1;
        }
    }
}

int dns_name_decode(const unsigned char *packet, int *offset,
                    char *out_buf, int buf_size) {
    int jumped = 0;
    int ptr_countdown = 10;
    int pos = *offset;
    int out_len = 0;
    int label_len;
    int i;

    if (pos >= DNS_MAX_MESSAGE) {
        return -1;
    }

    for (;;) {
        if ((packet[pos] & 0xC0) == 0xC0) {
            uint16_t ptr;

            if (ptr_countdown-- == 0) {
                return -1;
            }
            ptr = ((uint16_t)(packet[pos] & 0x3F) << 8) | packet[pos + 1];
            if (!jumped) {
                *offset += 2;
                jumped = 1;
            }
            pos = (int)ptr;
            continue;
        }

        label_len = packet[pos];
        if (label_len == 0) {
            if (!jumped) {
                *offset = pos + 1;
            }
            if (out_len > 0) {
                out_buf[out_len - 1] = '\0';
            } else {
                out_buf[0] = '\0';
            }
            return 0;
        }
        if (label_len > DNS_MAX_LABEL_LEN) {
            return -1;
        }
        pos++;
        if (out_len + label_len + 1 >= buf_size) {
            return -1;
        }
        for (i = 0; i < label_len; i++) {
            out_buf[out_len++] = (char)packet[pos++];
        }
        out_buf[out_len++] = '.';
    }
}

int dns_name_encode(const char *name, unsigned char *out_buf, int buf_size) {
    int written = 0;
    int label_start = 0;
    int i;
    int name_len;
    int label_len;

    if (name == NULL) {
        return -1;
    }
    name_len = (int)strlen(name);
    if (name_len == 0 || name_len > DNS_MAX_NAME_LEN) {
        return -1;
    }

    for (i = 0; i <= name_len; i++) {
        if (name[i] == '.' || name[i] == '\0') {
            label_len = i - label_start;
            if (label_len == 0 && name[i] == '.') {
                return -1;
            }
            if (label_len > DNS_MAX_LABEL_LEN) {
                return -1;
            }
            if (written + 1 + label_len + 1 > buf_size) {
                return -1;
            }
            out_buf[written++] = (unsigned char)label_len;
            memcpy(&out_buf[written], &name[label_start], (size_t)label_len);
            written += label_len;
            label_start = i + 1;
            if (name[i] == '\0') {
                break;
            }
        }
    }

    if (written + 1 > buf_size) {
        return -1;
    }
    out_buf[written++] = 0x00;
    return written;
}
