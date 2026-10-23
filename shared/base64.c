/*
 * s0P0wn3d — Implémentation Base64.
 */

#include "base64.h"
#include <stdlib.h>
#include <string.h>

static const char b64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *base64_encode(const uint8_t *data, size_t len) {
    size_t out_len = 4 * ((len + 2) / 3);
    char *out = malloc(out_len + 1);
    if (!out) return NULL;

    size_t i, j;
    for (i = 0, j = 0; i < len;) {
        int bytes_read = 0;
        uint32_t octet_a = 0, octet_b = 0, octet_c = 0;

        if (i < len) { octet_a = data[i++]; bytes_read++; }
        if (i < len) { octet_b = data[i++]; bytes_read++; }
        if (i < len) { octet_c = data[i++]; bytes_read++; }

        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        out[j++] = b64_table[(triple >> 18) & 0x3F];
        out[j++] = b64_table[(triple >> 12) & 0x3F];
        out[j++] = bytes_read > 1 ? b64_table[(triple >> 6) & 0x3F] : '=';
        out[j++] = bytes_read > 2 ? b64_table[triple & 0x3F] : '=';
    }
    out[j] = '\0';
    return out;
}

size_t base64_decoded_len(const char *input) {
    size_t len = strlen(input);
    size_t padding = 0;
    if (len >= 2 && input[len - 1] == '=') padding++;
    if (len >= 2 && input[len - 2] == '=') padding++;
    return (len / 4) * 3 - padding;
}

static int b64_char_val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

size_t base64_decode(const char *input, uint8_t *output) {
    size_t len = strlen(input);
    size_t out_idx = 0;
    size_t i;

    for (i = 0; i < len; i += 4) {
        int a = b64_char_val(input[i]);
        int b = b64_char_val(input[i + 1]);
        int c = (i + 2 < len && input[i + 2] != '=') ?
                b64_char_val(input[i + 2]) : 0;
        int d = (i + 3 < len && input[i + 3] != '=') ?
                b64_char_val(input[i + 3]) : 0;

        if (a < 0 || b < 0) break;

        uint32_t triple = (a << 18) | (b << 12) | (c << 6) | d;

        output[out_idx++] = (uint8_t)(triple >> 16);
        if (i + 2 < len && input[i + 2] != '=')
            output[out_idx++] = (uint8_t)(triple >> 8);
        if (i + 3 < len && input[i + 3] != '=')
            output[out_idx++] = (uint8_t)(triple);
    }
    return out_idx;
}
