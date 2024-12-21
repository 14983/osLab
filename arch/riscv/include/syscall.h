#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "stdint.h"

#define SYS_OPENAT  56
#define SYS_CLOSE   57
#define SYS_LSEEK   62
#define SYS_WRITE 64
#define SYS_READ 63
#define SYS_GETPID 172

uint64_t sys_getpid();
uint64_t sys_write(int64_t fd, const char *buf, uint64_t len);
uint64_t sys_read(int64_t fd, char *buf, uint64_t len);
int64_t sys_openat(int dfd, const char *filename, int flags); // dfd will not be used
int64_t sys_close(int64_t fd);
int64_t sys_lseek(int64_t fd, uint64_t offset, uint64_t whence);

#endif