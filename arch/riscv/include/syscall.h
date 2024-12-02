#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "stdint.h"

#define SYS_WRITE 64
#define SYS_GETPID 172

uint64_t getpid();

uint64_t write(uint64_t fd, const char* buf, uint64_t count);

#endif