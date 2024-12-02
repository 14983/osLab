#include "stdint.h"
#include "printk.h"
#include "defs.h"
#include "string.h"
#include "mm.h"

extern uint64_t _stext;
extern uint64_t _etext;
extern uint64_t _srodata;
extern uint64_t _erodata;
extern uint64_t _sdata;
extern uint64_t _edata;

/* early_pgtbl: map memory from 0x80000000 to 0xffffffe000000000 */
uint64_t early_pgtbl[512] __attribute__((__aligned__(0x1000)));

void setup_vm() {
    /* 
     * 1. 由于是进行 1GiB 的映射，这里不需要使用多级页表 
     * 2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
     *     high bit 可以忽略
     *     中间 9 bit 作为 early_pgtbl 的 index
     *     低 30 bit 作为页内偏移，这里注意到 30 = 9 + 9 + 12，即我们只使用根页表，根页表的每个 entry 都对应 1GiB 的区域
     * 3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    **/
    // need to set 2 early_pgtbl: 
    early_pgtbl[(PHY_START >> 30) % 512] = PGTBL_VALID | PGTBL_R | PGTBL_W | PGTBL_X | ((PHY_START >> 12 << 10) & (((uint64_t)1 << 54) - 1));
    early_pgtbl[(VM_START  >> 30) % 512] = PGTBL_VALID | PGTBL_R | PGTBL_W | PGTBL_X | ((PHY_START >> 12 << 10) & (((uint64_t)1 << 54) - 1));
    printk("...setup_vm done!\n");
}

/* swapper_pg_dir: kernel pagetable 根目录，在 setup_vm_final 进行映射 */
uint64_t swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

void create_mapping(uint64_t *pgtbl, uint64_t va, uint64_t pa, uint64_t sz, uint64_t perm);
void setup_vm_final() {
    memset(swapper_pg_dir, 0x0, PGSIZE);

    // No OpenSBI mapping required
    // mapping kernel text X|-|R|V
    create_mapping(swapper_pg_dir, (uint64_t)&_stext, (uint64_t)&_stext - PA2VA_OFFSET, (uint64_t)&_etext - (uint64_t)&_stext, PGTBL_X | PGTBL_R | PGTBL_VALID);

    // mapping kernel rodata -|-|R|V
    create_mapping(swapper_pg_dir, (uint64_t)&_srodata, (uint64_t)&_srodata - PA2VA_OFFSET, (uint64_t)&_erodata - (uint64_t)&_srodata, PGTBL_R | PGTBL_VALID);

    // mapping other memory -|W|R|V
    create_mapping(swapper_pg_dir, (uint64_t)&_sdata, (uint64_t)&_sdata - PA2VA_OFFSET, (uint64_t)PHY_END + (uint64_t)PA2VA_OFFSET - (uint64_t)&_sdata, PGTBL_W | PGTBL_R | PGTBL_VALID);

    // set satp with swapper_pg_dir
    csr_write(satp, (((uint64_t)swapper_pg_dir - PA2VA_OFFSET)>> 12) & (((uint64_t)1 << 44) - 1) | ((uint64_t)8 << 60));

    // flush TLB
    asm volatile("sfence.vma zero, zero");
    printk("...setup_vm_final done!\n");
    return;
}


/* 创建多级页表映射关系 */
/* 不要修改该接口的参数和返回值 */
void create_mapping(uint64_t *pgtbl, uint64_t va, uint64_t pa, uint64_t sz, uint64_t perm) {
    /*
     * pgtbl 为根页表的基地址
     * va, pa 为需要映射的虚拟地址、物理地址
     * sz 为映射的大小，单位为字节
     * perm 为映射的权限（即页表项的低 8 位）_stext
     * 
     * 创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
     * 可以使用 V bit 来判断页表项是否存在
    **/
    printk(BLUE "DBG in `create_mapping` " CLEAR "pgtbl: %llx, va: %llx, pa: %llx, sz: %llx, perm: %d\n", pgtbl, va, pa, sz, perm);

    uint64_t size = PGROUNDUP(sz) >> 12;
    uint64_t offset, *base_addr, *tmp;
    for (uint64_t idx = 0; idx < size; idx++, va += 4096, pa += 4096) {
        // first level
        base_addr = pgtbl;
        offset = (va >> 30) & (((uint64_t)1 << 9) - 1);
        if ((base_addr[offset] & PGTBL_VALID) == (uint64_t)0) {
            tmp = (uint64_t *)kalloc();
            base_addr[offset] = PGTBL_VALID | (((uint64_t)tmp - PA2VA_OFFSET) >> 12 << 10);
        }
        // second level
        base_addr = (uint64_t *)((base_addr[offset] >> 10 << 12) + PA2VA_OFFSET);
        offset = (va >> 21) & (((uint64_t)1 << 9) - 1);
        if ((base_addr[offset] & PGTBL_VALID) == (uint64_t)0) {
            tmp = (uint64_t *)kalloc();
            base_addr[offset] = PGTBL_VALID | (((uint64_t)tmp - PA2VA_OFFSET) >> 12 << 10);
        }
        // third level
        base_addr = (uint64_t *)((base_addr[offset] >> 10 << 12) + PA2VA_OFFSET);
        offset = (va >> 12) & (((uint64_t)1 << 9) - 1);
        base_addr[offset] = perm | ((uint64_t)pa >> 12 << 10);
    }
}

void find_phy(uint64_t *va) {
    uint64_t *base_address, offset1, offset2, offset3, tmp;
    base_address = swapper_pg_dir;
    offset1 = ((uint64_t)va >> 30) & (((uint64_t)1 << 9) - 1);
    offset2 = ((uint64_t)va >> 21) & (((uint64_t)1 << 9) - 1);
    offset3 = ((uint64_t)va >> 12) & (((uint64_t)1 << 9) - 1);
    printk(BLUE "DBG in `find_phy` " CLEAR "%llx %llx, %llx, %llx\n", va, offset1, offset2, offset3);
    tmp = base_address[offset1];
    base_address = (uint64_t *)((tmp >> 10 << 12) + PA2VA_OFFSET);
    printk("level 1 page: %llx, base address: %llx\n", tmp, (uint64_t)base_address - PA2VA_OFFSET);
    tmp = base_address[offset2];
    base_address = (uint64_t *)((tmp >> 10 << 12) + PA2VA_OFFSET);
    printk("level 2 page: %llx, base address: %llx\n", tmp, (uint64_t)base_address - PA2VA_OFFSET);
    tmp = base_address[offset3];
    base_address = (uint64_t *)(tmp >> 10 << 12);
    printk("level 3 page: %llx, final address: %llx\n", tmp, base_address);
}