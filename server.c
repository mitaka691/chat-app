#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "net.h"

#define PORT 5555
#define MAX_CLIENTS 10

int clients[MAX_CLIENTS]; // spisuk
int client_count = 0; // broi


// prashta suobshtenie kum vsichki
void broadcast(unsigned char *buffer, uint32_t len, int sender) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i] != sender) {
        	printf("Forwarding message from %d to %d (%u bytes)\n", sender, clients[i], len);
            send_packet(clients[i], buffer, len);
          
        }
       
    }
}

// obrabotva komunikaciq s edin klient
void *handle_client(void *arg) {
    int sock = *(int *)arg;
    free(arg); // osvobozhdava pamet
	printf("Handling client socket: %d\n", sock);
    unsigned char buffer[4096];
    uint32_t len;
	
	// poluchava i preprashta paketi
    while (recv_packet(sock, buffer, &len) == 0) {
        printf("Received packet from client %d (%u bytes)\n", sock, len);
		broadcast(buffer, len, sock);
        
    }
	
    close(sock);
    return NULL;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); // TCP soket
    
	if (server_fd < 0) {
    perror("socket failed");
    exit(1);
}

	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

	struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = htons(PORT),.sin_addr.s_addr = INADDR_ANY};

// bind kum port
if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("bind failed");
    exit(1);
}

// listen
if (listen(server_fd, 5) < 0) {
    perror("listen failed");
    exit(1);
}

printf("Server running on port %d...\n", PORT);

    while (1) {
        int *client_sock = malloc(sizeof(int));
        
        // priema nov klient
		*client_sock = accept(server_fd, NULL, NULL);
		
		if (*client_sock < 0) {
    	perror("accept failed");
    	free(client_sock);
    	continue;
}
		
		printf("Client connected! Socket = %d\n", *client_sock);
		
	if (client_count < MAX_CLIENTS) {
		
		// vliza v spisuka
    	clients[client_count++] = *client_sock;
} else {
    	printf("Max clients reached!\n");
    	close(*client_sock);
    	free(client_sock);
    continue;
}
		pthread_t t;
		// startira thread
		pthread_create(&t, NULL, handle_client, client_sock);
		pthread_detach(t);
    }
    
}