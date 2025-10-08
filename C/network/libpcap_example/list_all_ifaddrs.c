#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <pcap.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>

static int count_prefix_len(const struct sockaddr *nm) {
    if (!nm)
        return -1;
    const uint8_t *b = NULL;
    int bytes = 0;
    if (nm->sa_family == AF_INET) {
        b = (const uint8_t *)&((const struct sockaddr_in *)nm)->sin_addr;
        bytes = 4;
    } else if (nm->sa_family == AF_INET6) {
        b = (const uint8_t *)&((const struct sockaddr_in6 *)nm)->sin6_addr;
        bytes = 16;
    } else {
        return -1;
    }

    int cnt = 0;
    for (int i = 0; i < bytes; i++) {
        uint8_t v = b[i];
        for (int j = 7; j >= 0; j--) {
            if (v & (1u << j))
                cnt++;
            else
                return cnt;
        }
    }

    return cnt;
}

static void print_addr(const pcap_addr_t *a) {
    if (!a || !a->addr)
        return;

    if (a->addr->sa_family == AF_INET) {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &((struct sockaddr_in *)a->addr)->sin_addr, ip, sizeof(ip));
        int p = count_prefix_len(a->netmask);
        if (ip[0])
            printf("    IPv4: %s%s\n", ip, p >= 0 ? "" : " (mask unknown) ");
        if (p >= 0)
            printf("        /%d\n", p);
    } else if (a->addr->sa_family == AF_INET6) {
        char out[INET6_ADDRSTRLEN + IF_NAMESIZE + 2];
        const struct sockaddr_in6 *sa6 = (const struct sockaddr_in6 *)a->addr;
        char base[INET6_ADDRSTRLEN];
        if (inet_ntop(AF_INET6, &sa6->sin6_addr, base, sizeof(base))) {
            if (sa6->sin6_scope_id) {
                char ifname[IF_NAMESIZE];
                if (if_indextoname(sa6->sin6_scope_id, ifname))
                    snprintf(out, sizeof(out), "%s%%%s", base, ifname);
                else
                    snprintf(out, sizeof(out), "%s%%%u", base, sa6->sin6_scope_id);
            } else {
                snprintf(out, sizeof(out), "%s", base);
            }
            int p = count_prefix_len(a->netmask);
            printf("    IPv6: %s%s\n", out, p >= 0 ? "" : " (prefix unknown) ");
            if (p >= 0)
                printf("        /%d\n", p);
        }
    }
}

int main() {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *alldevs = NULL;

    if (pcap_findalldevs(&alldevs, errbuf) == -1 || !alldevs) {
        fprintf(stderr, "pcap_findalldevs: %s\n", errbuf[0] ? errbuf : "no devices");
        return 1;
    }

    for (pcap_if_t *d = alldevs; d; d = d->next) {
        printf("Device: %s%s\n", d->name, (d->flags & PCAP_IF_LOOPBACK) ? " (loopback) " : "");
        for (pcap_addr_t *a = d->addresses; a; a = a->next) {
            print_addr(a);
        }
        puts("");
    }

    pcap_freealldevs(alldevs);
    return 0;
}