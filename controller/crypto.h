#ifndef S0P_CRYPTO_H
#define S0P_CRYPTO_H

#include "../shared/protocol.h"

int crypto_init(const char *key_dir);
void crypto_cleanup(void);
char *crypto_get_public_key_pem(void);
int crypto_decrypt_aes_key(const char *encrypted_key_b64, uint8_t *aes_key);
int crypto_aes_encrypt(const uint8_t *key, const uint8_t *plaintext, size_t pt_len,
                       char **nonce_b64, char **payload_b64, char **tag_b64);
int crypto_aes_decrypt(const uint8_t *key, const char *nonce_b64,
                       const char *payload_b64, const char *tag_b64,
                       uint8_t *plaintext, size_t max_len);

#endif
