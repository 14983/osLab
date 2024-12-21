# 浙江大学 24 年秋冬操作系统实验

---

本仓库包含了浙江大学 24 年秋冬操作系统实验的 Lab 1 ~ 6。

使用环境：

+ Debian GNU/Linux 12
+ qemu-system-riscv 1:9.1.2+ds-1~bpo12+1
+ opensbi 1.5.1-1~bpo12+2

使用 Debian Backports 安装，命令：

```bash
$ sudo apt -t stable-backports install qemu-system-riscv
```

| 序号 | 名称 | 最后一次提交 ID |
| :---: | :---: | :---: |
| Lab 1 | RV64 内核引导与时钟中断处理 | `f427723` |
| Lab 2 | RV64 内核线程调度 | `fd724cf` |
| Lab 3 | RV64 虚拟内存管理 | `ef75b8c` |
| Lab 4 | RV64 用户态程序 | `efd67a4` |
| Lab 5 | RV64 缺页异常处理与 fork 机制 | `9b88a6a` |
| Lab 6 | VFS & FAT32 文件系统 | `683938a` |

如果要查看某个 Lab 最后提交时的状态，请使用：

```bash
$ git checkout <commit ID>
$ # for example, git checkout 683938a
```

注意事项：

0. **可能会有 Bug，请谨慎参考**
1. Lab 2 测试调度的选项为 `make TEST_SCHED=1`
2. Lab 3 不含等值映射的版本在 `2fd5559`
3. Lab 5 没有实现 COW
4. Lab 5 的测试选项详见 `user/main.c`
5. Lab 6 基于 Lab 4，在分支 `bonus`
6. 本仓库的 Lab 6 中不含磁盘映像文件（`disk.img`）

