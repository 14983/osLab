#ifndef __STRING_H__
#define __STRING_H__

#include "stdint.h"

void *memset(void *, int, uint64_t);
int strlen(const char *str);
void memcpy(char *dst, const char *src, uint64_t n);
int memcmp(const char *x, const char *y, uint64_t n);

#endif
