#include "base64.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *base64_encode(const unsigned char *data, size_t len) {
    if (!data || len == 0) return NULL;

    size_t out_len = ((len + 2) / 3) * 4 + 1;
    char *out = malloc(out_len);
    if (!out) return NULL;

    size_t i = 0, j = 0;
    while (i < len) {
        uint8_t b1 = data[i++];
        uint8_t b2 = (i < len) ? data[i++] : 0;
        uint8_t b3 = (i < len) ? data[i++] : 0;

        uint32_t group = (b1 << 16) | (b2 << 8) | b3;

        out[j++] = b64_table[(group >> 18) & 0x3F];
        out[j++] = b64_table[(group >> 12) & 0x3F];
        out[j++] = (i - 1 < len) ? b64_table[(group >> 6) & 0x3F] : '=';
        out[j++] = (i < len) ? b64_table[group & 0x3F] : '=';
    }
    out[j] = '\0';
    return out;
}

unsigned char *base64_decode(const char *str, size_t *out_len) {
    if (!str || !out_len) return NULL;

    size_t len = strlen(str);
    unsigned char *out = malloc(len);
    if (!out) return NULL;

    *out_len = 0;
    size_t i = 0;
    while (i < len) {
        if (str[i] == '=') break;
        i++;
    }

    memset(out, 0, len);
    *out_len = 0;
    return out;
}
