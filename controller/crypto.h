/*
 * s0P0wn3d — Module cryptographique du contrôleur (OpenSSL).
 *
 * Gère :
 *   - Génération/chargement de la paire de clés RSA-4096
 *   - Déchiffrement RSA-OAEP de la clé AES envoyée par l'implant
 *   - Chiffrement/déchiffrement AES-256-GCM des payloads
 */

#ifndef S0P_CONTROLLER_CRYPTO_H
#define S0P_CONTROLLER_CRYPTO_H

#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <stdint.h>
#include <stddef.h>

#include "../shared/protocol.h"

/* Génère ou charge la paire de clés RSA depuis key_dir.
 * Retourne 0 en cas de succès. */
int crypto_init(const char *key_dir);

/* Libère les ressources crypto. */
void crypto_cleanup(void);

/* Retourne la clé publique RSA au format PEM (chaîne malloc-ée). */
char *crypto_get_public_key_pem(void);

/* Déchiffre la clé AES chiffrée avec RSA-OAEP.
 * Écrit les AES_KEY_SIZE bytes dans aes_key.
 * Retourne 0 en cas de succès. */
int crypto_decrypt_aes_key(const char *encrypted_key_b64, uint8_t *aes_key);

/* Chiffre un message avec AES-256-GCM.
 * Alloue et remplit nonce_b64, payload_b64, tag_b64 (à free par l'appelant).
 * Retourne 0 en cas de succès. */
int crypto_aes_encrypt(const uint8_t *aes_key, const uint8_t *plaintext,
                       size_t pt_len, char **nonce_b64, char **payload_b64,
                       char **tag_b64);

/* Déchiffre un message AES-256-GCM.
 * Retourne les bytes déchiffrés dans output (pré-alloué).
 * Retourne le nombre de bytes ou -1 en cas d'erreur. */
int crypto_aes_decrypt(const uint8_t *aes_key, const char *nonce_b64,
                       const char *payload_b64, const char *tag_b64,
                       uint8_t *output, size_t max_out);

#endif /* S0P_CONTROLLER_CRYPTO_H */
