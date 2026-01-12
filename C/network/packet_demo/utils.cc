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