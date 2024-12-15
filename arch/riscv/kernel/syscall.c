#include "syscall.h"
#include "stdint.h"
#include "sbi.h"
#include "proc.h"
#include "mm.h"
#include "defs.h"
#include "stddef.h"
#include "vm.h"
#include "printk.h"
#include "trap.h"

extern struct task_struct *current;
extern uint64_t nr_tasks;
extern struct task_struct *task[];
extern uint64_t swapper_pg_dir[];
extern void __ret_from_fork();

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

uint64_t do_fork(struct pt_regs *regs) {
    uint64_t parent_pid = sys_getpid();
    uint64_t child_pid  = ++nr_tasks;
    // set basic properties
    task[child_pid] = (struct task_struct*)kalloc();
    for (uint64_t i = 0; i < PGSIZE; i++)
        ((char*)(task[child_pid]))[i] = ((char*)(task[parent_pid]))[i];
    task[child_pid] -> pid = child_pid;
    (task[child_pid] -> mm).mmap = (struct vm_area_struct*)NULL;
    // set kernel page table
    task[child_pid] -> pgd = (uint64_t *)((uint64_t)kalloc() - PA2VA_OFFSET);
    uint64_t *pgd_va = (uint64_t *)((uint64_t)(task[child_pid] -> pgd) + PA2VA_OFFSET);
    for (int j = 256; j < 512; j++)
        (pgd_va)[j] = swapper_pg_dir[j];
    // set user page table
    for (struct vm_area_struct *p_vma = (task[parent_pid] -> mm).mmap; p_vma; p_vma = p_vma -> vm_next) {
        DBG("Copying user page table: start: %llx, end: %llx", p_vma -> vm_start, p_vma -> vm_end);
        do_mmap(&(task[child_pid] -> mm), p_vma -> vm_start, (uint64_t)(p_vma->vm_end) - (uint64_t)(p_vma->vm_start), p_vma -> vm_pgoff, p_vma -> vm_filesz, p_vma -> vm_flags);
        for (uint64_t tar_va = PGROUNDDOWN(p_vma -> vm_start); tar_va < PGROUNDUP(p_vma -> vm_end); tar_va += PGSIZE) {
            uint64_t *tar_pa = find_phy(task[parent_pid] -> pgd, (uint64_t*)tar_va);
            DBG("mapped address of %llx: %llx", tar_va, tar_pa);
            if (tar_pa == (uint64_t *)NULL) continue;
            uint64_t *new_pa = (uint64_t *)kalloc();
            for (uint64_t i = 0; i < PGSIZE; i++)
                ((char*)new_pa)[i] = ((char*)tar_pa + PA2VA_OFFSET)[i];
            uint64_t perm = PGTBL_VALID | PGTBL_U |
                ((p_vma -> vm_flags & VM_EXEC)  ? PGTBL_X : 0) |
                ((p_vma -> vm_flags & VM_WRITE) ? PGTBL_W : 0) |
                ((p_vma -> vm_flags & VM_READ)  ? PGTBL_R : 0);
            create_mapping(task[child_pid] -> pgd, (uint64_t)tar_va, (uint64_t)new_pa - PA2VA_OFFSET, PGSIZE, perm);
        }
    }
    // set kernel/user sp and ra
    (task[child_pid] -> thread).ra = (uint64_t)__ret_from_fork;
    (task[child_pid] -> thread).sp = ((uint64_t)regs & (PGSIZE - 1)) + (uint64_t)task[child_pid];
    ((struct pt_regs*)((task[child_pid] -> thread).sp)) -> x10 = 0;
    (task[child_pid] -> thread).sscratch = csr_read(sscratch);
    return child_pid;
}