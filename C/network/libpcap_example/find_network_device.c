#include <stdio.h>
#include <pcap.h>

int main() {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *alldevs;
    pcap_if_t *device;

    // すべてのネットワークデバイスを取得
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        fprintf(stderr, "Error finding devices: %s\n", errbuf);
        return 1;
    }

    // 最初のデバイスを取得
    device = alldevs;
    if (device == NULL) {
        printf("No devices found.\n");
        return 1;
    }

    printf("Using device: %s\n", device->name);

    // 使用が終わったら解放
    pcap_freealldevs(alldevs);

    return 0;
}