#ifndef S0P_IMPL_CRYPTO_H
#define S0P_IMPL_CRYPTO_H

#include "../shared/protocol.h"
#include <stdint.h>

int impl_crypto_load_pubkey(const char *pem);
void impl_crypto_gen_aes_key(uint8_t *key);
char *impl_crypto_encrypt_aes_key(const uint8_t *aes_key);
int impl_crypto_aes_encrypt(const uint8_t *key, const uint8_t *plaintext, size_t pt_len,
                            char **nonce_b64, char **payload_b64, char **tag_b64);
int impl_crypto_aes_decrypt(const uint8_t *key, const char *nonce_b64,
                            const char *payload_b64, const char *tag_b64,
                            uint8_t *plaintext, size_t max_len);
void impl_crypto_cleanup(void);

#endif
