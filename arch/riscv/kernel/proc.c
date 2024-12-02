#include "mm.h"
#include "defs.h"
#include "proc.h"
#include "stdlib.h"
#include "printk.h"

extern void __dummy();
extern void __switch_to(struct task_struct *prev, struct task_struct *next);

extern uint64_t swapper_pg_dir[];
extern char _sramdisk[];
extern char _eramdisk[];

struct task_struct *idle;           // idle process
struct task_struct *current;        // 指向当前运行线程的 task_struct
struct task_struct *task[NR_TASKS]; // 线程数组，所有的线程都保存在此

void create_mapping(uint64_t *pgtbl, uint64_t va, uint64_t pa, uint64_t sz, uint64_t perm);

void task_init() {
    srand(2024);

    // 1. 调用 kalloc() 为 idle 分配一个物理页
    // 2. 设置 state 为 TASK_RUNNING;
    // 3. 由于 idle 不参与调度，可以将其 counter / priority 设置为 0
    // 4. 设置 idle 的 pid 为 0
    // 5. 将 current 和 task[0] 指向 idle

    void *idle_task_page = kalloc();
    idle = (struct task_struct*)idle_task_page;
    idle -> state = TASK_RUNNING;
    idle -> counter = 0;
    idle -> priority = 0;
    idle -> pid = 0;
    current = idle;
    task[0] = idle;

    // 1. 参考 idle 的设置，为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, 此外，counter 和 priority 进行如下赋值：
    //     - counter  = 0;
    //     - priority = rand() 产生的随机数（控制范围在 [PRIORITY_MIN, PRIORITY_MAX] 之间）
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 thread_struct 中的 ra 和 sp
    //     - ra 设置为 __dummy（见 4.2.2）的地址
    //     - sp 设置为该线程申请的物理页的高地址

    for (uint64_t i = 1; i < NR_TASKS; i++){
        task[i] = (struct task_struct*)kalloc();
        task[i] -> state = TASK_RUNNING;
        task[i] -> pid = i;
        task[i] -> counter = 0;
        task[i] -> priority = rand() % (PRIORITY_MAX - PRIORITY_MIN + 1) + PRIORITY_MIN;
        (task[i] -> thread).ra = (uint64_t)(__dummy);
        (task[i] -> thread).sp = (uint64_t)((void *)task[i] + PGSIZE);
        (task[i] -> thread).sepc = USER_START;
        (task[i] -> thread).sstatus = (1U << SSTATUS_SUM) | (0U << SSTATUS_SPP) | (1U << SSTATUS_SPIE);
        (task[i] -> thread).sscratch = USER_END;
        task[i] -> pgd = (uint64_t *)((uint64_t)kalloc() - PA2VA_OFFSET);
        uint64_t *pgd_va = (uint64_t *)((uint64_t)(task[i] -> pgd) + PA2VA_OFFSET);
        // copy kernel page table to user page table
        for (int j = 256; j < 512; j++)
            (pgd_va)[j] = swapper_pg_dir[j];
        // user mode stack
        uint64_t u_mode_stack_pa = (uint64_t)kalloc() - PA2VA_OFFSET;
        create_mapping(pgd_va, USER_END - PGSIZE, u_mode_stack_pa, PGSIZE, PGTBL_W | PGTBL_R | PGTBL_U | PGTBL_VALID);
        // u app (copied)
        uint64_t *uapp_copy_pa = (uint64_t *)(alloc_pages(PGROUNDUP(_eramdisk - _sramdisk) >> 12) - PA2VA_OFFSET);
        for (char *j = (char *)((uint64_t)uapp_copy_pa + PA2VA_OFFSET), *k = _sramdisk; k < _eramdisk; j++, k++) {
            *j = *k;
        }
        create_mapping(pgd_va, 0, (uint64_t)uapp_copy_pa, _eramdisk - _sramdisk, PGTBL_R | PGTBL_W | PGTBL_X | PGTBL_U | PGTBL_VALID);
        // test: print user page table
        /*
        for (int j = 0; j < 512; j++) {
            printk("%d %llx\n", j, (pgd_va)[j]);
        }
        */
    }
    printk("...task_init done!\n");
}

void dummy() {
    uint64_t MOD = 1000000007;
    uint64_t auto_inc_local_var = 0;
    int last_counter = -1;
    while (1) {
        if ((last_counter == -1 || current->counter != last_counter) && current->counter > 0) {
            if (current->counter == 1) {
                --(current->counter);   // forced the counter to be zero if this thread is going to be scheduled
            }                           // in case that the new counter is also 1, leading the information not printed.
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
        }
    }
}

void do_timer() {
    if (current == task[0]) { // idle task
        schedule();
        return;
    }
    if (current -> counter == 0) { // time slice is 0
        schedule();
        return;
    }
    current -> counter--;
    if (current -> counter == 0) {
        schedule();
        return;
    }
}

void schedule() {
    uint64_t time_slice, chosen_task;
    while (1) {
        time_slice = 0;
        for (int i = 0; i < NR_TASKS; i++) {
            if (task[i] -> counter > time_slice) {
                time_slice = task[i] -> counter;
                chosen_task = i;
            }
        }
        if (time_slice > 0) break;
        printk("\n");
        for (int i = 1; i < NR_TASKS; i++) {
            task[i] -> counter = task[i] -> priority;
            printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n", task[i] -> pid, task[i] -> priority, task[i] -> counter);
        }
    }
    switch_to(task[chosen_task]);
}

void switch_to(struct task_struct *next) {
    if (current != next) {
        printk("\nswitch to [PID = %d PRIORITY = %d COUNTER = %d]\n", next -> pid, next -> priority, next -> counter);
        struct task_struct *tmp = current;
        current = next;
        __switch_to(tmp, next);
    }
}