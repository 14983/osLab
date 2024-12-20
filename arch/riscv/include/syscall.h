#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "stdint.h"

#define SYS_WRITE 64
#define SYS_READ 63
#define SYS_GETPID 172

uint64_t sys_getpid();
uint64_t sys_write(uint64_t fd, const char *buf, uint64_t len);
uint64_t sys_read(uint64_t fd, char *buf, uint64_t len);

#endif