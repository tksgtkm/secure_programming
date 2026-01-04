#ifndef ETHERNET_H
#define ETHERNET_H

#include <cstdio>
#include "net.h"

#define ETHER_TYPE_IP 0x0800
#define ETHER_TYPE_ARP 0x8060
#define ETHER_TYPE_IPV6 0x86dd

#define ETHERNET_HEADER_SIZE 14
#define ETHERNET_ADDRESS_LEN 6

const uint8_t ETHERNET_ADDRESS_BROADCAST[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

struct ethernet_header {
    uint8_t dest_addr[6];
    uint8_t src_addr[6];
    uint16_t type;
} __attribute__((packed));

void ethernet_input(net_device *dev, uint8_t *buffer, ssize_t len);

struct my_buf;

void ethernet_encapsulate_output(net_device *dev, const uint8_t *dest_addt, my_buf *payload_mybuf, uint16_t ether_type);

#endif