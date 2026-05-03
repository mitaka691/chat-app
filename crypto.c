#include "crypto.h"
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <string.h>


// AES-256-GCM kriptirane
int aes_gcm_encrypt(unsigned char *plaintext, int plaintext_len,
            unsigned char *key,
            unsigned char *iv,
            unsigned char *ciphertext,
            unsigned char *tag) {

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len, ciphertext_len;

    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv);

    EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len);
    ciphertext_len = len;

    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    ciphertext_len += len;

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);

    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}

// AES dekriptirane s proverka
int aes_gcm_decrypt(unsigned char *ciphertext, int ciphertext_len,
            unsigned char *tag,
            unsigned char *key,
            unsigned char *iv,
            unsigned char *plaintext) {

      EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    int ret;

    // kontekts
    if (!(ctx = EVP_CIPHER_CTX_new())) {
        printf("CTX error\n");
        return -1;
    }

    // AES algorith
    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) {
        printf("Init error\n");
        return -1;
    }

    // IV razmer (12 baita)
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, NULL)) {
        printf("IV len error\n");
        return -1;
    }

    // key i IV
    if (!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv)) {
        printf("Key/IV error\n");
        return -1;
    }

    //  dekriptira
    if (!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
        printf("Decrypt update error\n");
        return -1;
    }
    plaintext_len = len;

    // TAG 
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag)) {
        printf("TAG set error\n");
        return -1;
    }

    // integrity!
    ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

    EVP_CIPHER_CTX_free(ctx);

    if (ret > 0) {
        plaintext_len += len;
        plaintext[plaintext_len] = '\0';
        return plaintext_len; 
    } else {
        // podpraveni danni
        return -1;
    }
}
 
// RSA key (2048 bita)
EVP_PKEY *generate_rsa_key() {
    EVP_PKEY *pkey = NULL;
    EVP_PKEY_CTX *ctx = NULL;

    ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (!ctx) return NULL;

    if (EVP_PKEY_keygen_init(ctx) <= 0) return NULL;
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0) return NULL;

    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) return NULL;

    EVP_PKEY_CTX_free(ctx);
    return pkey;
}

// serializira public key v REM format 
int serialize_public_key(EVP_PKEY *pkey, unsigned char **out) {
    BIO *bio = BIO_new(BIO_s_mem());

    PEM_write_bio_PUBKEY(bio, pkey);

    size_t len = BIO_pending(bio);

    *out = malloc(len);
    BIO_read(bio, *out, len);

    BIO_free(bio);
    return len;
}

// deserializira kliocha v baitove
EVP_PKEY *deserialize_public_key(unsigned char *data, int len) {
    BIO *bio = BIO_new_mem_buf(data, len);
    EVP_PKEY *pkey = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
    BIO_free(bio);
    return pkey;
}

// kriptirane s public key
int rsa_encrypt(unsigned char *data, int data_len, EVP_PKEY *pubkey, unsigned char *out) {

    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(pubkey, NULL);
    size_t outlen;

    EVP_PKEY_encrypt_init(ctx);
    EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);

    EVP_PKEY_encrypt(ctx, NULL, &outlen, data, data_len);
    EVP_PKEY_encrypt(ctx, out, &outlen, data, data_len);

    EVP_PKEY_CTX_free(ctx);
    return outlen;
}

// dekriptira s private key
int rsa_decrypt(unsigned char *enc_data, int enc_len, EVP_PKEY *privkey, unsigned char *out) {

    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(privkey, NULL);
    size_t outlen;

    EVP_PKEY_decrypt_init(ctx);
    EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);

    EVP_PKEY_decrypt(ctx, NULL, &outlen, enc_data, enc_len);
    EVP_PKEY_decrypt(ctx, out, &outlen, enc_data, enc_len);

    EVP_PKEY_CTX_free(ctx);
    return outlen;
}