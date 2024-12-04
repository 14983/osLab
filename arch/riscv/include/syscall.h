#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "stdint.h"

#define SYS_WRITE 64
#define SYS_GETPID 172

uint64_t sys_getpid();

uint64_t sys_write(unsigned int fd, const char* buf, uint64_t count);

#endif