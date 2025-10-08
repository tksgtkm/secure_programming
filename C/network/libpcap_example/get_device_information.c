#include <stdio.h>
#include <pcap.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

int main() {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *alldevs = NULL;
    pcap_if_t *dev = NULL;

    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        fprintf(stderr, "pcap_findalldevs error: %s\n", errbuf);
        return 1;
    }

    if (alldevs == NULL) {
        fprintf(stderr, "No devices found\n");
        return 1;
    }

    dev = alldevs;
    printf("Device: %s\n", dev->name);

    char ip[INET_ADDRSTRLEN];
    char mask[INET_ADDRSTRLEN];
    int found_v4 = 0;

    for (pcap_addr_t *a = dev->addresses; a != NULL; a = a->next) {
        if (a->addr && a->addr->sa_family == AF_INET) {
            struct sockaddr_in *sa = (struct sockaddr_in *)a->addr;
            struct sockaddr_in *nm = (struct sockaddr_in *)a->netmask;

            if (inet_ntop(AF_INET, &sa->sin_addr, ip, sizeof(ip)) == NULL) {
                perror("inet_ntop ip");
                continue;
            }
            if (nm && inet_ntop(AF_INET, &nm->sin_addr, mask, sizeof(mask)) == NULL) {
                perror("inet_nto mask");
                strcpy(mask, "(unknown)");
            }

            found_v4 = 1;
            break;
        }
    }

    if (found_v4) {
        printf("IP address: %s\n", ip);
        printf("Subnet mask: %s\n", mask);
    } else {
        printf("No IPv4 address found on this device\n");
    }

    pcap_freealldevs(alldevs);
    return 0;
}