#ifndef __PRINTK_H__
#define __PRINTK_H__

#include "stddef.h"

#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN "\033[1;36m"
#define CLEAR "\033[0m"

#define bool _Bool
#define true 1
#define false 0

int printk(const char *, ...);

#if LOG
#define Log(format, ...) \
    printk("\33[1;35m[%s,%d,%s] " format "\33[0m\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)
#else
#define Log(format, ...);
#endif

#if LOG
#define Err(format, ...) \
    printk("\33[1;31m[%s,%d,%s] " format "\33[0m\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)
#else
#define Err(format, ...);
#endif

#endif