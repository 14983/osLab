#include "syscall.h"
#include "stdint.h"
#include "sbi.h"
#include "proc.h"

extern struct task_struct *current;
extern struct task_struct *task[];

uint64_t getpid() {
    for (int i = 0; i < NR_TASKS; i++)
        if (current == task[i])
            return i;
    return -1;
}

uint64_t write(uint64_t fd, const char* buf, uint64_t count) {
    if (fd == 1U) { // print to console
        for (int i = 0; i < count; i++)
            sbi_debug_console_write_byte((uint8_t)buf[i]);
        return count;
    }
}