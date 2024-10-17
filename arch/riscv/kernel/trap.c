#include "stdint.h"
#include "printk.h"
#include "sbi.h"
#include "proc.h"

void trap_handler(uint64_t scause, uint64_t sepc) {
    // 通过 `scause` 判断 trap 类型
    // 如果是 interrupt 判断是否是 timer interrupt
    // 如果是 timer interrupt 则打印输出相关信息，并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.3.4 节
    // 其他 interrupt / exception 可以直接忽略，推荐打印出来供以后调试
    if (scause & ((uint64_t)1 << 63) == (uint64_t)1 << 63) {
        // interrupt
        if ((scause & (((uint64_t)1 << 63) - 1)) == 5) {
            // timer interrupt
            // printk("timer interrupt, time: %llu\n", get_cycles());
            clock_set_next_event();
            do_timer();
        } else {
            printk("unknown interrupt code: 0x%llx\n", (scause & (((uint64_t)1 << 63) - 1)));
        }
    } else {
        // exception
        printk("exception code: 0x%llx, sepc: 0x%llx\n", scause, sepc);
    }
}