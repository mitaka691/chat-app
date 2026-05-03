#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_PACKET 4096

// poluchava len baita (mozhe recv da vurne chastichno)
int recv_all(int sock, unsigned char *buf, int len) {
    int total = 0;
    while (total < len) {
        int n = recv(sock, buf + total, len - total, 0);
        if (n <= 0) return -1;
        total += n;
    }
    return total;
}

// izprashta tochno len baita (ako send e chastichen
int send_all(int sock, const unsigned char *buf, int len) {
    int total = 0;
    while (total < len) {
        int n = send(sock, buf + total, len - total, 0);
        if (n <= 0) return -1;
        total += n;
    }
    return total;
}

// klient se svurzva
int connect_to_server(const char *ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(1);
    }

    return sock;
}

// startira se servera
int start_server(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    return server_fd;
}

int accept_client(int server_fd) {
    return accept(server_fd, NULL, NULL);
}

// PACKET API - prashta paket ot 4 baita + danni
int send_packet(int sock, const unsigned char *data, uint32_t len) {
    uint32_t net_len = htonl(len); // network byte order

	// purvo dalzhina posle danni
    if (send_all(sock, (unsigned char*)&net_len, 4) < 0)
        return -1;

    if (send_all(sock, data, len) < 0)
        return -1;

    return EXIT_SUCCESS;
}

// poluchava paket
int recv_packet(int sock, unsigned char *buffer, uint32_t *len) {
    uint32_t net_len;

	// chete dalzhinata
    if (recv_all(sock, (unsigned char*)&net_len, 4) <= 0)
        return -1;

    *len = ntohl(net_len); // preobrazuva q kum hosta

    if (*len > MAX_PACKET)
        return -1; // zashtita ot prepulvane

    if (recv_all(sock, buffer, *len) <= 0)
        return -1; // chete danni

    return EXIT_SUCCESS;
}