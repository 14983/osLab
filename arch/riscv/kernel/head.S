    .extern start_kernel
    .extern mm_init
    .extern task_init
    .section .text.init
    .globl _start
_start:
    # ------------------
    # - your code here -
    # ------------------
    # initialize stack
    la sp, boot_stack_top
    # set stvec = _traps
    la t0, _traps
    csrw stvec, t0
    # initialize memory & task
    jal mm_init
    jal task_init
    # set sie[STIE] = 1
    csrr t0, sie
    li t1, 0x00000020
    or t0, t0, t1
    csrw sie, t0
    # set first time interrupt
    rdtime a0
    li t0, 10000000
    add a0, a0, t0
    add a1, x0, x0
    add a2, x0, x0
    add a3, x0, x0
    add a4, x0, x0
    add a5, x0, x0
    add a6, x0, x0
    li a7, 0x54494d45
    ecall
    # set sstatus[SIE] = 1
    csrr t0, sstatus
    li t1, 0x00000002
    or t0, t0, t1
    csrw sstatus, t0
    # jump to start_kernel
    jal start_kernel
    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 * 8 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top:
