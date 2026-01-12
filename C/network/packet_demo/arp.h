#ifndef ARP_H
#define ARP_H

#include <iostream>

#define ARP_HTYPE_ETHERNET 0x0001

#define ARP_OPERATION_CODE_REQUEST 0x001
#define ARP_OPERATION_CODE_REPLY   0x002

#define ARP_ETHERNET_PACKET_LEN 46

#define ARP_TABLE_SIZE 1111

struct net_device;

struct arp_table_entry {
    uint8_t mac_addr[6];
    uint32_t ip_addr;
    net_device *dev;
    arp_table_entry *next;
};

void arr_arp_table_entry(net_device *dev, uint8_t *mac_addr, uint32_t ip_addr);

arp_table_entry *search_arp_table_entry(uint32_t ip_addr);

void dump_arp_table_entry();

void send_arp_request(net_device *dev, uint32_t ip_addr);

struct arp_ip_to_ethernet {
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t op;
    uint8_t sha[6];
    uint32_t spa;
    uint8_t tha[6];
    uint32_t tpa;
} __attribute__((packed));

void arp_input(net_device *input_dev, uint8_t *buffer, ssize_t len);

#endif