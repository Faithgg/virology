/*
 * s0P0wn3d — Module cryptographique de l'implant (OpenSSL).
 *
 * Gère :
 *   - Chargement de la clé publique RSA du contrôleur
 *   - Chiffrement RSA-OAEP de la clé AES pour l'enregistrement
 *   - Chiffrement/déchiffrement AES-256-GCM des payloads
 */

#ifndef S0P_IMPLANT_CRYPTO_H
#define S0P_IMPLANT_CRYPTO_H

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <stdint.h>
#include <stddef.h>
#include "../shared/protocol.h"

/* Charge une clé publique RSA depuis un string PEM. Retourne 0 si OK. */
int impl_crypto_load_pubkey(const char *pem);

/* Génère une clé AES-256 aléatoire. */
void impl_crypto_gen_aes_key(uint8_t *key);

/* Chiffre la clé AES avec la clé publique RSA (OAEP).
 * Retourne une string base64 malloc-ée, ou NULL en cas d'erreur. */
char *impl_crypto_encrypt_aes_key(const uint8_t *aes_key);

/* Chiffre un message avec AES-256-GCM.
 * Alloue nonce_b64, payload_b64, tag_b64 (à free par l'appelant). */
int impl_crypto_aes_encrypt(const uint8_t *aes_key, const uint8_t *plaintext,
                            size_t pt_len, char **nonce_b64,
                            char **payload_b64, char **tag_b64);

/* Déchiffre un message AES-256-GCM.
 * Retourne le nombre de bytes ou -1 en cas d'erreur. */
int impl_crypto_aes_decrypt(const uint8_t *aes_key, const char *nonce_b64,
                            const char *payload_b64, const char *tag_b64,
                            uint8_t *output, size_t max_out);

/* Libère les ressources. */
void impl_crypto_cleanup(void);

#endif /* S0P_IMPLANT_CRYPTO_H */
