#include "string.h"
#include "stdint.h"

void *memset(void *dest, int c, uint64_t n) {
    char *s = (char *)dest;
    for (uint64_t i = 0; i < n; ++i) {
        s[i] = c;
    }
    return dest;
}

int strlen(const char *str) {
    int len = 0;
    while (*str++)
        len++;
    return len;
}

void memcpy(char *dst, const char *src, uint64_t n) {
    for (uint64_t i = 0; i < n; i++)
        dst[i] = src[i];
}

int memcmp(const char *x, const char *y, uint64_t n) {
    for (uint64_t i = 0; i < n; i++)
        if (x[i] != y[i])
            return -1;
    return 0;
}