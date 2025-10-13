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
    int dlt;
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

static int get_l3_offset_and_etype(const u_char *p, size_t caplen, int dlt, size_t *l2_len, uint16_t *etype_out) {
    if (dlt == DLT_EN10MB) {
        return skip_vlan(p, caplen, l2_len, etype_out);
    } else if (dlt == DLT_LINUX_SLL) {
        if (caplen < 16)
            return -1;
        *l2_len = 16;
        *etype_out = ntohs(*(const uint16_t *)(p + 14));
        return 0;
    } else if (dlt == DLT_LINUX_SLL2) {
        if (caplen < 20)
            return -1;
        *l2_len = 20;
        *etype_out = ntohs(*(const uint16_t *)(p + 0));
        return 0;
    }

    return -2;
}

static int ipv6_locate_l4(const u_char *base, size_t caplen, size_t ipv6_off, size_t *l4_off, uint8_t *nh_out) {
    if (caplen < ipv6_off + sizeof(struct ip6_hdr))
        return -1;
    const struct ip6_hdr *ip6 = (const struct ip6_hdr*)(base + ipv6_off);
    uint8_t nh = ip6->ip6_nxt;
    size_t off = ipv6_off + sizeof(struct ip6_hdr);

    while (1) {
        if (nh == IPPROTO_TCP || nh == IPPROTO_UDP || nh == IPPROTO_ICMPV6) {
            *l4_off = off;
            *nh_out = nh;
            return 0;
        }

        if (caplen < off + 2)
            return -1;
        
        if (nh == IPPROTO_FRAGMENT) {
            if (caplen < off + 8)
                return -1;
            const u_char *frag = base + off;
            nh = *(frag);
            off += 8;
            continue;
        } else if (nh == IPPROTO_AH) {
            if (caplen < off + 2)
                return -1;
            const u_char *ah = base + off;
            uint8_t nxt = ah[0];
            uint8_t paylen = ah[1];
            size_t hdrlen = (size_t)(paylen + 2) * 4;
            if (caplen < off + hdrlen)
                return -1;
            nh = nxt;
            off += hdrlen;
            continue;
        } else if (nh == IPPROTO_ESP) {
            return -1;
        } else {
            const u_char *ext = base + off;
            uint8_t nxt = ext[0];
            uint8_t extlen = ext[1];
            size_t hdrlen = (size_t)(extlen + 1) * 8;
            if (caplen < off + hdrlen)
                return -1;
            nh = nxt;
            off += hdrlen;
            continue;
        }
    }
}

static void on_packet(u_char *uargs, const struct pcap_pkthdr *h, const u_char *p) {
    user_args_t *ua = (user_args_t *)uargs;

    if (h->caplen < sizeof(struct ether_header)) {
        if (ua && ua->verbose)
            printf("Too short for Ethernet header\n");
            return;
    }

    uint16_t etype = 0;
    size_t l2 = 0;
    int rc = get_l3_offset_and_etype(p, h->caplen, ua ? ua->dlt : DLT_EN10MB, &l2, &etype);
    if (rc != 0) {
        if (ua && ua->verbose) {
            if (rc == -2)
                printf("Unsupported DLT=%d (e.g radiotap)\n", ua->dlt);
            else
                printf("L2 parse error\n");
        }
        return;
    }

    if (ua && ua->verbose) {
        printf("caplen=%u len=%u L2=%zu ether=0x%04x\n", h->caplen, h->len, l2, etype);
    }

    if (etype == ETHERTYPE_IP) {
        if (h->caplen < l2 + sizeof(struct ip))
            return;
        const struct ip *ip = (const struct ip *)(p + l2);
        int ip_hl = ip->ip_hl * 4;
        if (ip_hl < 20 || (size_t)(l2 + ip_hl) > h->caplen)
            return;
        
        char src[INET_ADDRSTRLEN], dst[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ip->ip_src, src, sizeof(src));
        inet_ntop(AF_INET, &ip->ip_dst, dst, sizeof(dst));

        const u_char *l4 = p + l2 + ip_hl;
        uint8_t proto = ip->ip_p;

        if (proto == IPPROTO_TCP) {
            if (h->caplen , (size_t)(l2 + ip_hl + sizeof(struct tcphdr)))
                return;
            const struct tcphdr *th = (const struct tcphdr *)l4;
            int thl = th->th_off * 4;
            if (thl < 20 || (size_t)(l2 + ip_hl + thl) > h->caplen)
                return;
            size_t payload_len = 0;
            uint16_t ip_tot = ntohs(ip->ip_len);
            if (ip_tot >= (uint16_t)(ip_hl + thl)) {
                payload_len = ip_tot - ip_hl - thl;
                size_t rest = h->caplen - (l2 + ip_hl + thl);
                if (payload_len > rest)
                    payload_len - rest;
            }
            printf("IPv4 TCP %s:%u -> %s:%u payload=%zu\n",
            src, ntohs(th->th_sport), dst, ntohs(th->th_dport), payload_len);
        } else if (proto == IPPROTO_UDP) {
            if (h->caplen < (size_t)(l2 + ip_hl + sizeof(struct udphdr *)))
                return;
            const struct udphdr *uh = (const struct udphdr *)l4;
            uint16_t ulen = ntohs(uh->len);
            size_t payload_len = 0;
            if (ulen >= sizeof(struct udphdr))
                payload_len = ulen - sizeof(struct udphdr);
            size_t rest = h->caplen - (l2 + ip_hl + sizeof(struct udphdr));
            if (payload_len > rest)
                payload_len = rest;
            printf("IPv4 UDP %s:%u -> %s:%u payload=%zu",
            src, ntohs(uh->uh_sport), dst, ntohs(uh->uh_dport), payload_len);
        } else {
            if (ua && ua->verbose)
                printf("IPv4 other proto=%u\n", proto);
        }
        return;
    }

    if (etype == ETHERTYPE_IPV6) {
        size_t l4off = 0;
        uint8_t nh = 0;
        if (ipv6_locate_l4(p, h->caplen, l2, &l4off, &nh) != 0) {
            if (ua && ua->verbose)
                printf("IPv6: failed to locate L4\n");
                return;
        }

        const struct ip6_hdr *ip6 = (const struct ip6_hdr*)(p + l2);
        char src[INET6_ADDRSTRLEN], dst[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &ip6->ip6_src, src, sizeof(src));
        inet_ntop(AF_INET6, &ip6->ip6_dst, dst, sizeof(dst));

        if (nh == IPPROTO_TCP) {
            if (h->caplen < l4off + sizeof(struct tcphdr))
                return;
            const struct tcphdr *th = (const struct tcphdr *)(p + l4off);
            int thl = th->th_off * 4;
            if (thl < 20 || h->caplen < l4off + (size_t)thl)
                return;

            uint16_t ip6_plens = ntohs(ip6->ip6_plen);
            size_t payload_len = 0;
            if (ip6_plens > (l4off - (l2 + sizeof(struct ip6_hdr)) + (size_t)thl)) {
                size_t l4_payload_from_ip = ip6_plens - (l4off - (l2 + sizeof(struct ip6_hdr))) - thl;
                payload_len = l4_payload_from_ip;
                size_t rest = h->caplen - (l4off + thl);
                if (payload_len > rest)
                    payload_len = rest;
            } else {
                size_t rest = h->caplen - (l4off + thl);
                payload_len = rest;
            }
            printf("IPv6 TCP [%s]:%u -> [%s]:%u payload=%zu\n",
            src, ntohs(th->th_sport), dst, ntohs(th->th_dport), payload_len);
        } else if (nh == IPPROTO_UDP) {
            if (h->caplen < l4off + sizeof(struct udphdr*))
                return;
            const struct udphdr *uh = (const struct udphdr *)(p + l4off);
            uint16_t ulen = ntohs(uh->len);
            size_t payload_len = (ulen >= sizeof(struct udphdr)) ? (ulen - sizeof(struct udphdr)) : 0;
            size_t rest = h->caplen - (l4off + sizeof(struct udphdr));
            if (payload_len > rest)
                payload_len = rest;
            printf("IPv6 UDP [%s]:%u -> [%s]:%u payload=%zu\n",
            src, ntohs(uh->uh_sport), dst, ntohs(uh->uh_dport), payload_len);
        } else {
            if (ua && ua->verbose)
                printf("IPv6 other next-header=%u\n", nh);
        }
        return;
    }

    if (ua && ua->verbose)
        printf("Non-IP EtherType=0x%04x\n", etype);
}

static void parse_args(int argc, char **argv, const char **ifname, int *count, int *verbose, char **filter) {
    *ifname = NULL;
    *count = -1;
    *verbose = 0;
    *filter = NULL;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-i") && i+1 < argc) {
            *ifname = argv[++i];
        } else if (!strcmp(argv[i], "-c") && i+1 < argc) {
            *count = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-v")) {
            *verbose = 1;
        } else {
            size_t len = 0;
            for (int j = i; j < argc; ++j)
                len += strlen(argv[j]) + 1;
            *filter = (char *)malloc(len + 1);
            *filter[0] = '\0';
            for (int j = i; j < argc; ++j) {
                strcat(*filter, argv[j]);
                if (j + 1 < argc)
                    strcat(*filter, " ");
            }
            break;
        }
    }
}

int main(int argc, char **argv) {
    /*
    # まずは何でも表示（10個）
    sudo ./sniffer -i wlp9s0 -c 10 -v
    
    # TCP だけ（HTTP/HTTPS トラフィックを別ターミナルで発生させる）
    sudo ./sniffer -i wlp9s0 -c 50 "tcp"
    
    # どれがIFか分からない時
    sudo ./sniffer -c 10 -v          # 最初のIF自動選択
    sudo ./sniffer -i any -c 10      # any でまとめ取り（環境によっては便利）

    sudo ./find_payload -i wlp9s0 -c 20 'tcp or (udp and port 53)'
    # または TCPだけ
    sudo ./find_payload -i wlp9s0 -c 20 'tcp'
    */
    char errbuf[PCAP_ERRBUF_SIZE];
    const char *dev = NULL;
    char *devdup = NULL;
    int packet_count = 200;
    int verbose = 0;
    char *filter_str = NULL;

    parse_args(argc, argv, &dev, &packet_count, &verbose, &filter_str);

    if (!dev) {
        pcap_if_t *alldevs = NULL;
        if (pcap_findalldevs(&alldevs, errbuf) == -1 || !alldevs) {
            fprintf(stderr, "findalldevs: %s\n", errbuf[0] ? errbuf : "no devices");
            free(filter_str);
            return 1;
        }
        dev = alldevs->name;
        devdup = strdup(alldevs->name);
        if (verbose)
            printf("Auto-selected devices: %s\n", dev);
        pcap_freealldevs(alldevs);
        dev = devdup;
    }

    bpf_u_int32 ip_raw = 0, mask_raw = 0;
    if (pcap_lookupnet(dev, &ip_raw, &mask_raw, errbuf) == -1) {
        if (verbose)
            fprintf(stderr, "pcap_lookupnet: %s (continuing)\n", errbuf);
        mask_raw = 0;
    }

    int snaplen = 65535;
    int promisc = 0;
    int timeout_ms = 1000;
    pcap_t *h = pcap_open_live(dev, snaplen, promisc, timeout_ms, errbuf);
    if (!h) {
        fprintf(stderr, "open_live(%s): %s\n", dev, errbuf);
        free(filter_str);
        return 2;
    }

    int dlt = pcap_datalink(h);
    if (verbose)
        printf("DLT=%d on %s\n", dlt, dev);

    if (!(dlt == DLT_EN10MB || dlt == DLT_LINUX_SLL || dlt == DLT_LINUX_SLL2)) {
        fprintf(stderr, "Unsupported datalink on %s (DLT=%d). Try another IF or monitor mode.\n", dev, dlt);
        pcap_close(h);
        free(filter_str);
        free(devdup);
        return 3;
    }

    struct bpf_program fp;
    if (filter_str && filter_str[0]) {
        if (pcap_compile(h, &fp, filter_str, 1, mask_raw) == -1) {
            fprintf(stderr, "pcap_compile error: %s\n", pcap_geterr(h));
            pcap_close(h);
            free(filter_str);
            return 4;
        }
        if (pcap_setfilter(h, &fp) == -1) {
            fprintf(stderr, "pcap_setfilter error: %s\n", pcap_geterr(h));
            pcap_freecode(&fp);
            pcap_close(h);
            free(filter_str);
            return 5;
        }
        pcap_freecode(&fp);
    }

    user_args_t ua = {
        .verbose = verbose,
        .dlt = dlt
    };
    printf("Capturing on %s%s%s\n",
           dev,
           filter_str ? " with filter: " : "",
           filter_str ? filter_str : "");

    int rc = pcap_loop(h, packet_count, on_packet, (u_char*)&ua);
    if (rc == -1) {
        fprintf(stderr, "pcap_loop error: %s\n", pcap_geterr(h));
    }

    pcap_close(h);
    free(devdup);
    free(filter_str);
    return 0;
}