#include <stdio.h>
#include <time.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <pcap.h>

static void print_mac(const u_char *m) {
    printf("%02x:%02x:%02x:%02x:%02x:%02x", m[0], m[1], m[2], m[3], m[4], m[5]);
}

static void print_packet_info(const u_char *packet, const struct pcap_pkthdr *hdr) {
    char tbuf[64];
    time_t sec = (time_t)hdr->ts.tv_sec;
    struct tm lt;
    localtime_r(&sec, &lt);
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S %Z", &lt);

    printf("Time received        : %s\n", tbuf);
    printf("Packet length        : %u\n", hdr->len);
    printf("Packet-capture length: %u\n", hdr->caplen);

    if (hdr->caplen < sizeof(struct ether_header)) {
        printf("Captured data too short for Either header\n");
        return;
    }

    const struct ether_header *eth = (const struct ether_header *)packet;
    uint16_t etype = ntohs(eth->ether_type);

    printf("EtherType: 0x%04x ", etype);
    if (etype == ETHERTYPE_IP) puts("(IPv4)");
    else if (etype == ETHERTYPE_ARP) puts("(ARP)");
    else if (etype == ETHERTYPE_IPV6) puts("(IPv6)");
    else puts("(Other)");

    printf("Source MAC        :");
    print_mac(eth->ether_shost);
    printf("\n");

    printf("Destination MAC   :");
    print_mac(eth->ether_dhost);
    printf("\n");
}

int main(int argc, char *argv[]) {
    // sudo ./live_capture wlp9s0
    char errbuf[PCAP_ERRBUF_SIZE];
    const char *device = NULL;

    if (argc >= 2) {
        device = argv[1];
    } else {
        pcap_if_t *alldevs = NULL;
        if (pcap_findalldevs(&alldevs, errbuf) == -1 || !alldevs) {
            fprintf(stderr, "Error finding; devices: %s\n", errbuf[0] ? errbuf : "no devices");
            return 1;
        }

        device = alldevs->name;
        pcap_freealldevs(alldevs);
    }

    int snaplen = 65535;
    int promisc = 1;
    int timeout_ms = 10000;
    pcap_t *handle = pcap_open_live(device, snaplen, promisc, timeout_ms, errbuf);
    if (!handle) {
        fprintf(stderr, "Could not open device %s: %s\n", device, errbuf);
        return 2;
    }

    if (pcap_datalink(handle) != DLT_EN10MB) {
        fprintf(stderr, "Device %s is not Ethernet (DLT=%d)\n", device, pcap_datalink(handle));
        pcap_close(handle);
        return 3;
    }

    printf("Capturing on: %s\n", device);

    const u_char *packet = NULL;
    struct pcap_pkthdr *hdr = NULL;
    int rc = pcap_next_ex(handle, &hdr, &packet);
    if (rc == 1 && packet && hdr) {
        print_packet_info(packet, hdr);
    } else if (rc == 0) {
        printf("Timeout reached: no packet captured\n");
    } else if (rc == -1) {
        fprintf(stderr, "pcap_next_ex error: %s\n", pcap_geterr(handle));
    } else if (rc == -2) {
        fprintf(stderr, "pcap_next_ex: loop terminated by pcap_breakloop\n");
    }

    pcap_close(handle);
    return 0;
}