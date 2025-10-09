#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <pcap.h>

#ifndef ETH_P_8021Q
#define ETH_P_8021Q 0x8100
#endif
#ifndef ETH_P_8021AD
#define ETH_P_8021AD 0x88A8
#endif
#ifndef ETH_Q_QINQ
#define ETH_P_QINQ 0x9100
#endif

typedef struct {
    int verbose;
} user_args_t;

static void print_mac(const u_char *m) {
    printf("%02x:%02x:%02x:%02x:%02x:%02x", m[0], m[1], m[2], m[3], m[4], m[5]);
}

static int skip_vlan(const u_char *pkt, size_t caplen, size_t *l2_len, uint16_t *etype_out) {
    if (caplen < sizeof(struct ether_header))
        return -1;
    
    const struct ether_header *eth = (const struct ether_header *)pkt;
    size_t off = sizeof(struct ether_header);
    uint16_t etype = ntohs(eth->ether_type);

    for (int i = 0; i < 8; i++) {
        if (etype == ETH_P_8021Q || etype == ETH_P_8021AD || etype == ETH_P_QINQ) {
            if (caplen < off + 4)
                return -1;
            etype = ntohs(*(const uint16_t *)(pkt + off + 2));
            off += 4;
        } else {
            break;
        }
    }
    *l2_len = off;
    *etype_out = etype;
    return 0;
}