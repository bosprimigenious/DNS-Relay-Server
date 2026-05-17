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

int dns_parse_query(const unsigned char *packet, int packet_len, char *qname,
                    int qname_size, uint16_t *qtype, uint16_t *qclass) {
    const dns_header_t *hdr;
    uint16_t qdcount;
    int offset;

    if (packet_len < 12) {
        return -1;
    }

    hdr = (const dns_header_t *)packet;
    qdcount = ntohs(hdr->qdcount);
    if (qdcount != 1) {
        return -1;
    }
    if ((ntohs(hdr->flags.value) >> 15) & 1) {
        return -1;
    }

    offset = 12;
    if (dns_name_decode(packet, &offset, qname, qname_size) != 0) {
        return -1;
    }
    if (offset + 4 > packet_len) {
        return -1;
    }

    *qtype = ntohs(*(const uint16_t *)(packet + offset));
    *qclass = ntohs(*(const uint16_t *)(packet + offset + 2));
    return 0;
}

int dns_build_error_response(const unsigned char *query, int query_len,
                             unsigned char *response, int response_size,
                             uint8_t rcode) {
    dns_header_t *hdr;

    if (query_len < 12 || response_size < query_len) {
        return -1;
    }

    memcpy(response, query, (size_t)query_len);
    hdr = (dns_header_t *)response;
    dns_header_network_to_host(hdr);
    hdr->flags.bits.qr = 1;
    hdr->flags.bits.ra = 0;
    hdr->flags.bits.rcode = rcode;
    hdr->ancount = 0;
    hdr->nscount = 0;
    hdr->arcount = 0;
    dns_header_host_to_network(hdr);
    return query_len;
}

int dns_build_a_response(const unsigned char *query, int query_len,
                         unsigned char *response, int response_size,
                         struct in_addr ip, uint32_t ttl) {
    int offset;
    int qname_end_offset;
    int question_len;
    int response_len;
    dns_header_t *hdr;
    int off;

    if (query_len < 12) {
        return -1;
    }

    offset = 12;
    if (dns_name_skip(query, &offset) != 0) {
        return -1;
    }
    qname_end_offset = offset;
    question_len = qname_end_offset + 4 - 12;
    response_len = 12 + question_len + 16;

    if (response_len > response_size) {
        return -1;
    }

    memcpy(response, query, (size_t)(12 + question_len));
    hdr = (dns_header_t *)response;
    dns_header_network_to_host(hdr);
    hdr->flags.bits.qr = 1;
    hdr->flags.bits.ra = 0;
    hdr->flags.bits.rcode = 0;
    hdr->ancount = 1;
    hdr->nscount = 0;
    hdr->arcount = 0;
    dns_header_host_to_network(hdr);

    off = 12 + question_len;
    *(uint16_t *)(response + off) = htons(0xC00C);
    off += 2;
    *(uint16_t *)(response + off) = htons(DNS_TYPE_A);
    off += 2;
    *(uint16_t *)(response + off) = htons(DNS_QCLASS_IN);
    off += 2;
    *(uint32_t *)(response + off) = htonl(ttl);
    off += 4;
    *(uint16_t *)(response + off) = htons(4);
    off += 2;
    memcpy(response + off, &ip.s_addr, 4);

    return response_len;
}
