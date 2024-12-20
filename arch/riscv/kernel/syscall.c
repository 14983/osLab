#include "syscall.h"
#include "stdint.h"
#include "sbi.h"
#include "proc.h"
#include "fs.h"
#include "printk.h"

extern struct task_struct *current;

uint64_t sys_getpid() {
    return current -> pid;
}

uint64_t sys_write(uint64_t fd, const char *buf, uint64_t len) {
    int64_t ret;
    struct file *file = &(current->files->fd_array[fd]);
    if (file->opened == 0) {
        Err("File not opened\n");
        return ERROR_FILE_NOT_OPEN;
    }
    if (!((file -> perms) & FILE_WRITABLE)) {
        Err("File not writable, fd = %d\n", fd);
    }
    (file -> write)(file, buf, len);
    return ret;
}

uint64_t sys_read(uint64_t fd, char *buf, uint64_t len) {
    int64_t ret;
    struct file *file = &(current->files->fd_array[fd]);
    if (file->opened == 0) {
        Err("File not opened\n");
        return ERROR_FILE_NOT_OPEN;
    }
    if (!((file -> perms) & FILE_READABLE)) {
        Err("File not readable\n");
    }
    (file -> read)(file, buf, len);
    return ret;
}