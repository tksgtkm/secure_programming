#include <iostream>

#include "utils.h"

uint16_t swap_byte_order_16(uint16_t v) {
    return (v & 0x00ff) << 8 | (v & 0xff00) >> 8;
}

uint32_t swap_byte_order_32(uint32_t v) {
    return (v & 0x000000ff) << 24 |
           (v & 0x0000ff00) << 8 |
           (v & 0x00ff0000) >> 8 |
           (v & 0xff000000) >> 24;
}

uint16_t ntohs(uint16_t v) {
    return swap_byte_order_16(v);
}

uint16_t htons(uint16_t v) {
    return swap_byte_order_16(v);
}

uint32_t ntohl(uint32_t v) {
    return swap_byte_order_32(v);
}

uint32_t htonl(uint32_t v) {
    return swap_byte_order_32(v);
}

uint8_t ip_string_pool_index = 0;
char ip_string_pool[4][16];

const char *ip_ntoa(uint32_t in) {
    uint8_t a = in & 0x000000ff;
    uint8_t b = in >> 8 & 0x000000ff;
    uint8_t c = in >> 16 & 0x000000ff;
    uint8_t d = in >> 24 & 0x000000ff;
    ip_string_pool_index++;
    ip_string_pool_index %= 4;
    sprintf(ip_string_pool[ip_string_pool_index],
            "%d.%d.%d.%d", a, b, c, d);
    return ip_string_pool[ip_string_pool_index];
}

