export
CROSS	:=	riscv64-linux-gnu-
GCC		:=	$(CROSS)gcc
LD		:=	$(CROSS)ld
OBJCOPY	:=	$(CROSS)objcopy
OBJDUMP	:=	$(CROSS)objdump

ISA		:=	rv64imafd
ABI		:=	lp64

INCLUDE	:=	-I $(shell pwd)/include -I $(shell pwd)/arch/riscv/include
CF		:=	-march=$(ISA) -mabi=$(ABI) -mcmodel=medany -fno-builtin -ffunction-sections -fdata-sections -nostartfiles -nostdlib -nostdinc -static -lgcc -Wl,--nmagic -Wl,--gc-sections -g -fno-pie
CFLAG   :=  $(CF) $(INCLUDE)

BIOS    := default

.PHONY:all run debug clean
all: clean
	$(MAKE) -C user all
	$(MAKE) -C lib all
	$(MAKE) -C init all
	$(MAKE) -C arch/riscv all
	@echo -e '\n'Build Finished OK

run: all
	@echo Launch qemu...
	@qemu-system-riscv64 -nographic -machine virt -kernel vmlinux -bios $(BIOS) 

debug: all
	@echo Launch qemu for debug...
	@qemu-system-riscv64 -nographic -machine virt -kernel vmlinux -bios $(BIOS) -S -s

clean:
	$(MAKE) -C user clean
	$(MAKE) -C lib clean
	$(MAKE) -C init clean
	$(MAKE) -C arch/riscv clean
	$(shell test -f vmlinux && rm vmlinux)
	$(shell test -f vmlinux.asm && rm vmlinux.asm)
	$(shell test -f System.map && rm System.map)
	@echo -e '\n'Clean Finished
