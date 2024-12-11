#include "stdint.h"
#include "printk.h"
#include "sbi.h"
#include "proc.h"
#include "defs.h"
#include "syscall.h"

void trap_handler(uint64_t scause, uint64_t sepc, struct pt_regs *regs) {
    // 通过 `scause` 判断 trap 类型
    // 如果是 interrupt 判断是否是 timer interrupt
    // 如果是 timer interrupt 则打印输出相关信息，并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.3.4 节
    // 其他 interrupt / exception 可以直接忽略，推荐打印出来供以后调试
    // regs: = ra
    /*
    printk(
        RED "regs: \n"
        "sepc: 0x%llx, \n"
        "x31: 0x%llx, x30: 0x%llx, x29: 0x%llx, x28: 0x%llx, \n"
        "x27: 0x%llx, x26: 0x%llx, x25: 0x%llx, x24: 0x%llx, \n"
        "x23: 0x%llx, x22: 0x%llx, x21: 0x%llx, x20: 0x%llx, \n"
        "x19: 0x%llx, x18: 0x%llx, x17: 0x%llx, x16: 0x%llx, \n"
        "x15: 0x%llx, x14: 0x%llx, x13: 0x%llx, x12: 0x%llx, \n"
        "x11: 0x%llx, x10: 0x%llx, x9: 0x%llx, x8: 0x%llx, \n"
        "x7: 0x%llx, x6: 0x%llx, x5: 0x%llx, x4: 0x%llx, \n"
        "x3: 0x%llx, x1: 0x%llx\n" CLEAR,
        sepc,
        regs -> x31, regs -> x30, regs -> x29, regs -> x28,
        regs -> x27, regs -> x26, regs -> x25, regs -> x24,
        regs -> x23, regs -> x22, regs -> x21, regs -> x20,
        regs -> x19, regs -> x18, regs -> x17, regs -> x16,
        regs -> x15, regs -> x14, regs -> x13, regs -> x12,
        regs -> x11, regs -> x10, regs -> x9, regs -> x8,
        regs -> x7, regs -> x6, regs -> x5, regs -> x4,
        regs -> x3, regs -> x1
    );
    */
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
        if (scause == (uint64_t)0x8) { // Environment call from U-mode
            printk(RED "TRAP INFO: " CLEAR "Environment call from U-mode, SYSCALL_ID = %d\n", regs -> x17);
            regs -> sepc += 4;
            if (regs -> x17 == SYS_WRITE) {
                printk(GREEN "U-Mode write: " CLEAR);
                regs -> x10 = sys_write(regs -> x10, (char *)regs -> x11, regs -> x12);
            }
            else if (regs -> x17 == SYS_GETPID)
                regs -> x10 = sys_getpid();
        } else if (scause == 12 || scause == 13 || scause == 15) { // inst, read, write page fault
            if (scause == 12)
                printk(RED "TRAP INFO: " CLEAR "Instruction Page Fault\n");
            else if (scause == 13)
                printk(RED "TRAP INFO: " CLEAR "Load Page Fault\n");
            else
                printk(RED "TRAP INFO: " CLEAR "Store/AMO Page Fault\n");
            do_page_fault(regs);
        } else {
            printk(RED "TRAP INFO: " CLEAR "exception code: 0x%llx, sepc: 0x%llx\n", scause, sepc);
        }
    }
}