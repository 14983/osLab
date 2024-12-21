#include "stdint.h"
#include "printk.h"
#include "sbi.h"
#include "proc.h"
#include "defs.h"
#include "syscall.h"

struct pt_regs {
    uint64_t sepc;
    uint64_t x31, x30, x29, x28;
    uint64_t x27, x26, x25, x24;
    uint64_t x23, x22, x21, x20;
    uint64_t x19, x18, x17, x16;
    uint64_t x15, x14, x13, x12;
    uint64_t x11, x10, x9, x8;
    uint64_t x7, x6, x5, x4;
    uint64_t x3, x1;
};

void trap_handler(uint64_t scause, uint64_t sepc, struct pt_regs *regs) {
    if ((scause & ((uint64_t)1 << 63)) == (uint64_t)1 << 63) {
        // interrupt
        if ((scause & (((uint64_t)1 << 63) - 1)) == 5) {
            // timer interrupt
            // printk("timer interrupt, time: %llu\n", get_cycles());
            clock_set_next_event();
            do_timer();
        } else {
            printk(RED "TRAP INFO: " CLEAR "unknown interrupt code: 0x%llx\n", (scause & (((uint64_t)1 << 63) - 1)));
        }
    } else {
        // exception
        switch (regs->x17) {
            case SYS_CLOSE:
                regs->x10 = sys_close(regs->x10);
                break;
            case SYS_WRITE:
                regs->x10 = sys_write(regs->x10, (const char *)regs->x11, regs->x12);
                break;
            case SYS_READ:
                regs->x10 = sys_read(regs->x10, (char *)regs->x11, regs->x12);
                break;
            case SYS_LSEEK:
                regs->x10 = sys_lseek(regs->x10, regs->x11, regs->x12);
                break;
            case SYS_GETPID:
                regs->x10 = sys_getpid();
                break;
            case SYS_OPENAT:
                regs->x10 = sys_openat(regs->x10, (const char *)regs->x11, regs->x12);
                break;
            default:
                Err("not support syscall id = %d", regs->x17);
        }
        regs->sepc += 4;
    }
}