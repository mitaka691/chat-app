#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <openssl/rand.h>
#include "crypto.h"
#include "net.h"

#define PORT 5555


//globalni kliochove
unsigned char aes_key[32]; // za kriptirane/dekriptirane
EVP_PKEY *my_rsa; // nashiq RSA klioch 
int aes_ready = 0; // dali e gotov
int is_initiator = 0; // dali klienta generira kliocha
int peer_key_received = 0; // dali veche sme poluchili chuzhdiq publichen kliov

void *receive_thread(void *sock_ptr) {
    int sock = *(int*)sock_ptr;
    unsigned char buffer[4096];
		
    while (1) {
    uint32_t len;
	
	//poluchava paket ot servera
    if (recv_packet(sock, buffer, &len) != 0) {
        printf("Connection closed or error\n");
        break;
    }

    unsigned char type = buffer[0];
	unsigned char *data = buffer + 1;
	uint32_t data_len = len - 1;
	
	// PUBLIC KEY - obrabotka
if (type == TYPE_PUBLIC_KEY) {
    EVP_PKEY *peer = deserialize_public_key(data, data_len);
    
	// ignorira ako veche sme go poluchili
 	if (peer_key_received) {
        EVP_PKEY_free(peer);
        continue;
    }

	peer_key_received = 1;
	
    printf("Peer public key received\n");
    printf("Received packet type: %d, len: %u\n", type, len);

    // ako sme iniciqtor, generirame i prashtame AES
    if (is_initiator && !aes_ready) {
    	
        RAND_bytes(aes_key, 32);
        aes_ready = 1;
		
		printf("AES key generated: ");
		for (int i = 0; i < 32; i++) {
    	printf("%02x", aes_key[i]);
		}
		printf("\n");
		
		// kriptira AES s RSA
        unsigned char encrypted[512];
        int enc_len = rsa_encrypt(aes_key, 32, peer, encrypted);

        unsigned char pkt[512];
        pkt[0] = TYPE_AES_KEY;
        memcpy(pkt + 1, encrypted, enc_len);

        send_packet(sock, pkt, enc_len + 1);

        printf("AES key sent\n");
    }
	EVP_PKEY_free(peer);
    continue;
}

	// AES KEY - poluchavene (kriptiran na RSA)
	if (type == TYPE_AES_KEY) {
    unsigned char decrypted_key[32];

	// dekriptirane
    int dec_len = rsa_decrypt(data, data_len, my_rsa, decrypted_key);

    if (dec_len == 32) {
        memcpy(aes_key, decrypted_key, 32);
        aes_ready = 1;
        
        printf("AES key received: ");
		for (int i = 0; i < 32; i++) {
    	printf("%02x", aes_key[i]);
		}
		printf("\n");
        
        printf("AES key received and set!\n");
    } else {
        printf("RSA decrypt failed\n");
    }
    continue;
}

	// MESSAGE - obrabotka na suobshtenieto
	if (type == TYPE_MESSAGE) {

    // ako nqma klioch, go propuska
    int all_zero = 1;
    for (int i = 0; i < 32; i++) {
        if (aes_key[i] != 0) {
            all_zero = 0;
            break;
        }
    }

    if (all_zero) {
        printf("AES key not set yet, skipping message\n");
        continue;
    }

    if (data_len < 28) {
        printf("Invalid message packet\n");
        continue;
    }

    unsigned char iv[12];
    unsigned char tag[16];
    unsigned char plaintext[4096];

	// izvlicha IV i TAG
    memcpy(iv, data, 12);
    memcpy(tag, data + 12, 16);

    unsigned char *ciphertext = data + 28;
    int cipher_len = data_len - 28;

    int plain_len = aes_gcm_decrypt(ciphertext, cipher_len, tag, aes_key, iv, plaintext);

    if (plain_len < 0) {
        printf("Decrypt failed\n");
    } else {
        plaintext[plain_len] = '\0';
        printf("Message: %s\n", plaintext);
    	}
	}
	
	}
	return NULL;
}

int main(int argc, char *argv[]) {
    int sock = socket(AF_INET, SOCK_STREAM, 0); // pravi soket

    struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = htons(PORT), .sin_addr.s_addr = inet_addr("127.0.0.1")};


	// svurzva se kum servera
  if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("connect failed");
    return 1;
}
	printf("Connected to server!\n");

	// proverqva za iniciqtor
	if (argc > 1 && strcmp(argv[1], "init") == 0) {
    is_initiator = 1;
}
	
	//RSA key exchange

	my_rsa = generate_rsa_key();
	printf("RSA key generated\n");
	
	// startira thread za suobshteniq
	pthread_t t;
	pthread_create(&t, NULL, receive_thread, &sock);
	
	// izprashta publichen klioch
	unsigned char *pubkey_buf;
	int pubkey_len = serialize_public_key(my_rsa, &pubkey_buf);

	unsigned char packet[4096];
	packet[0] = TYPE_PUBLIC_KEY;

	memcpy(packet + 1, pubkey_buf, pubkey_len);

	send_packet(sock, packet, pubkey_len + 1);
	printf("Public key sent (%d bytes)\n", pubkey_len);
    
    char input[1024];
	
    while (1) {

	// izchakva
    if (!aes_ready) {
        usleep(100000);
        continue;
    }
	// chete klaviqturata
    if (!fgets(input, sizeof(input), stdin))
        break;

    unsigned char iv[12];
    unsigned char tag[16];
    unsigned char ciphertext[1024];

    RAND_bytes(iv, sizeof(iv)); // random IV

	// kriptira suobshtenieto
    int len = aes_gcm_encrypt((unsigned char*)input, strlen(input), aes_key, iv, ciphertext, tag);

    unsigned char packet[4096];

    packet[0] = TYPE_MESSAGE;
	
	//podrezhda paketite IV + TAG + DATA
    memcpy(packet + 1, iv, 12);
    memcpy(packet + 13, tag, 16);
    memcpy(packet + 29, ciphertext, len);

    send_packet(sock, packet, len + 29);
}
}