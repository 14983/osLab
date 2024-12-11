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

#define DBG(format, ...) \
    printk(BLUE "DBG in `%s`: " CLEAR format "\n", __func__, ##__VA_ARGS__)
#define ERR(format, ...) \
    printk(RED "ERR in `%s`: " CLEAR format "\n", __func__, ##__VA_ARGS__); \
    while(1)


#endif