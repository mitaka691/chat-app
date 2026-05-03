#ifndef NET_H
#define NET_H
#include <stdint.h>

#define TYPE_PUBLIC_KEY 1 // RSA public key
#define TYPE_AES_KEY    2 // AES key (kriptiran chrez RSA)
#define TYPE_MESSAGE    3 // kriptiranoto suobshtenie

int connect_to_server(const char *ip, int port);

int start_server(int port);

int accept_client(int server_fd);

// (length + data)
int send_packet(int sock, const unsigned char *data, uint32_t len);

int recv_packet(int sock, unsigned char *buffer, uint32_t *len);

#endif