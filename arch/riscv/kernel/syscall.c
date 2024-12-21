#include "syscall.h"
#include "stdint.h"
#include "sbi.h"
#include "proc.h"
#include "fs.h"
#include "printk.h"
#include "string.h"
#include "fat32.h"

extern struct task_struct *current;

uint64_t sys_getpid() {
    return current -> pid;
}

uint64_t sys_write(int64_t fd, const char *buf, uint64_t len) {
    int64_t ret;
    struct file *file = &(current->files->fd_array[fd]);
    if (fd < 0 || file->opened == 0) {
        Err("File not opened\n");
        return ERROR_FILE_NOT_OPEN;
    }
    if (!((file -> perms) & FILE_WRITABLE)) {
        Err("File not writable, fd = %d\n", fd);
    }
    ret = (file -> write)(file, buf, len);
    return ret;
}

uint64_t sys_read(int64_t fd, char *buf, uint64_t len) {
    int64_t ret;
    struct file *file = &(current->files->fd_array[fd]);
    if (fd < 0 || file->opened == 0) {
        Err("File not opened\n");
        return ERROR_FILE_NOT_OPEN;
    }
    if (!((file -> perms) & FILE_READABLE)) {
        Err("File not readable\n");
    }
    ret = (file -> read)(file, buf, len);
    return ret;
}

int64_t sys_openat(int dfd, const char *filename, int flags) {
    int64_t fd = 0;
    // search for an empty fd
    for (int i = 0; i < MAX_FILE_NUMBER; i++) {
        if (current->files->fd_array[i].opened == 0) {
            fd = i;
            break;
        }
    }
    if (fd == 0) return -1; // no empty fd
    // check the path
    if (memcmp(filename, "/fat32/", 7) != 0) return -1; // only support fat32
    // open the file
    struct fat32_file new_file_struct = fat32_open_file(filename);
    if (new_file_struct.cluster == 0) return -1; // file not found
    // set file properties
    current->files->fd_array[fd].opened = 1;
    current->files->fd_array[fd].perms = flags;
    current->files->fd_array[fd].cfo = 0;
    current->files->fd_array[fd].fs_type = FS_TYPE_FAT32;
    current->files->fd_array[fd].fat32_file = new_file_struct;
    if (flags & FILE_WRITABLE) {
        current->files->fd_array[fd].write = fat32_write;
    }
    if (flags & FILE_READABLE) {
        current->files->fd_array[fd].read = fat32_read;
    }
    current->files->fd_array[fd].lseek = fat32_lseek;
    return fd;
}

int64_t sys_close(int64_t fd) {
    struct file *file = &(current->files->fd_array[fd]);
    if (file->opened == 0) {
        Err("File not opened\n");
        return -1;
    }
    file -> opened = 0;
    return 0;
}

int64_t sys_lseek(int64_t fd, uint64_t offset, uint64_t whence) {
    struct file *file = &(current->files->fd_array[fd]);
    if (fd < 0 || file->opened == 0) {
        Err("File not opened\n");
        return -1;
    }
    printk("sys_lseek: fd = %d, offset = %d, whence = %d\n", fd, offset, whence);
    return (file -> lseek)(file, offset, whence);
}