#include "syscall.h"
#include "stdint.h"
#include "sbi.h"
#include "proc.h"

extern struct task_struct *current;

uint64_t sys_getpid() {
    return current -> pid;
}

uint64_t sys_write(unsigned int fd, const char* buf, uint64_t count) {
    if (fd == 1U) { // print to console
        for (int i = 0; i < count; i++)
            sbi_debug_console_write_byte((uint8_t)buf[i]);
        return count;
    }
}