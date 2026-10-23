#ifndef S0P_BASE64_H
#define S0P_BASE64_H

#include <stddef.h>

char *base64_encode(const unsigned char *data, size_t len);
unsigned char *base64_decode(const char *str, size_t *len);

#endif
