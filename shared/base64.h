/*
 * s0P0wn3d — Encodage/décodage Base64.
 */

#ifndef S0P_BASE64_H
#define S0P_BASE64_H

#include <stddef.h>
#include <stdint.h>

/* Encode des bytes en string base64. Retourne une chaîne malloc-ée. */
char *base64_encode(const uint8_t *data, size_t len);

/* Décode une string base64 en bytes. Retourne le nombre de bytes décodés.
 * output doit être pré-alloué avec au moins base64_decoded_len(input) bytes. */
size_t base64_decode(const char *input, uint8_t *output);

/* Taille maximale du buffer de sortie pour le décodage. */
size_t base64_decoded_len(const char *input);

#endif /* S0P_BASE64_H */
