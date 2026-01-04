#include <cstdint>
#include <fcntl.h>
#include <ifaddrs.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include "log.h"
#include "net.h"

#define IGNORE_INTERFACES {"lo", "bond0", "dummy0", "tunl0", "sit0"}

bool is_ignore_interface(const char *ifname) {
    char ignore_interfaces[][IF_NAMESIZE] = IGNORE_INTERFACES;
    for (int i = 0; i < sizeof(ignore_interfaces) / IF_NAMESIZE; i++) {
        if (strcmp(ignore_interfaces[i], ifname) == 0) {
            return true;
        }
    }
    return false;
}

net_device *get_net_device_by_name(const char *name) {
    net_device *dev;
    for (dev = net_dev_list; dev; dev = dev->next) {
        if (strcmp(dev->name, name) == 0) {
            return dev;
        }
    }
    return nullptr;
}

int net_device_transmit(struct net_device *dev, uint8_t *buffer, size_t len);

int net_device_poll(net_device *dev);

struct net_device_data {
    int fd;
};

int main() {
    struct ifreq ifr{};
    struct ifaddrs *addrs;

    getifaddrs(&addrs);

    for (ifaddrs *tmp = addrs; tmp; tmp = tmp->ifa_next) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET) {
            memset(&ifr, 0, sizeof(ifr));
            strcpy(ifr.ifr_name, tmp->ifa_name);

            if (is_ignore_interface(tmp->ifa_name)) {
                printf("Skipped to enable interface %s\n", tmp->ifa_name);
                continue;
            }

            int sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
            if (sock == -1) {
                LOG_ERROR("socket open failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            if (ioctl(sock, SIOCGIFINDEX, &ifr) == -1) {
                LOG_ERROR("ioctl SIOCGIFINDEX failed: %s\n", strerror(errno));
                close(sock);
                exit(EXIT_FAILURE);
            }

            sockaddr_ll addr{};
            memset(&addr, 0x00, sizeof(addr));
            addr.sll_family = AF_PACKET;
            addr.sll_protocol = htons(ETH_P_ALL);
            addr.sll_ifindex = ifr.ifr_ifindex;
            if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
                LOG_ERROR("bind failed: %s\n", strerror(errno));
                close(sock);
                exit(EXIT_FAILURE);
            }

            if (ioctl(sock, SIOCGIFHWADDR, &ifr) != 0) {
                LOG_ERROR("ioctl SIOCGIFHWADDR failed %s\n", strerror(errno));
                close(sock);
                continue;
            }

            auto *dev = (net_device *)calloc(1, sizeof(net_device) + sizeof(net_device_data));
            dev->ops.transmit = net_device_transmit;
            dev->ops.poll = net_device_poll;

            strcpy(dev->name, tmp->ifa_name);
            memcpy(dev->mac_addr, &ifr.ifr_hwaddr.sa_data[0], 6);
            ((net_device_data *)dev->data)->fd = sock;

            printf("Created device %s socket %d address %02x:%02x:%02x:%02x:%02x:%02x \n",
                    dev->name, sock, dev->mac_addr[0], dev->mac_addr[1], dev->mac_addr[2],
                    dev->mac_addr[3], dev->mac_addr[4], dev->mac_addr[5]);
            
            net_device *next;
            next = net_dev_list;
            net_dev_list = dev;
            dev->next = next;

            int val = fcntl(sock, F_GETFL, 0);
            fcntl(sock, F_SETFL, val | O_NONBLOCK);
        }
    }

    freeifaddrs(addrs);

    if (net_dev_list == nullptr) {
        LOG_ERROR("No interface is enabled!\n");
        exit(EXIT_FAILURE);
    }

    while (true) {
        for (net_device *dev = net_dev_list; dev; dev = dev->next) {
            dev->ops.poll(dev);
        }
    }

    printf("Goodbye\n");
    return 0;
}

int net_device_transmit(struct net_device *dev, uint8_t *buffer, size_t len) {
    send(((net_device_data *)dev->data)->fd, buffer, len, 0);
    return 0;
}

int net_device_poll(net_device *dev) {
    uint8_t recv_buffer[1550];
    ssize_t n = recv(
        ((net_device_data *)dev->data)->fd,
        recv_buffer,
        sizeof(recv_buffer), 0
    );
    if (n == -1) {
        if (errno == EAGAIN) {
            return 0;
        } else {
            return -1;
        }
    }

    printf("Received %lu bytes from %s: ", n, dev->name);
    for (int i = 0; i < n; ++i) {
        printf("%02x", recv_buffer[i]);
    }
    printf("\n");

    return 0;
}