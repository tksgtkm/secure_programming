#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "hacking.h"

int main() {
    int i, recv_length, sockfd;
    u_char buffer[9000];

    if ((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP)) == -1)
        fatal("colud not generate to socket");
    
    for (i = 0; i < 3; i++) {
        recv_length = recv(sockfd, buffer, 8000, 0);
        printf("get %d bytes packet\n", recv_length);
        dump(buffer, recv_length);
    }
}