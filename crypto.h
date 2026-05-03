#ifndef CRYPTO_H
#define CRYPTO_H

#include <openssl/evp.h>

// deklaracii za kriptografski funkcii
EVP_PKEY *generate_rsa_key();
int serialize_public_key(EVP_PKEY *pkey, unsigned char **out);
EVP_PKEY *deserialize_public_key(unsigned char *data, int len);
int aes_gcm_encrypt(unsigned char *plaintext, int plaintext_len,
        unsigned char *key,
        unsigned char *iv,
        unsigned char *ciphertext,
        unsigned char *tag);

int aes_gcm_decrypt(unsigned char *ciphertext, int ciphertext_len,
        unsigned char *tag,
        unsigned char *key,
        unsigned char *iv,
        unsigned char *plaintext);

int rsa_encrypt(unsigned char *data, int data_len, EVP_PKEY *pubkey, unsigned char *out);

int rsa_decrypt(unsigned char *enc_data, int enc_len, EVP_PKEY *privkey, unsigned char *out);

#endif