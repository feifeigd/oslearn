#include "tss.h"

#include <global.h> // PG_SIZE
#include <print.h>
#include <stdint.h>
#include <string.h>

// 任务状态段tss结构
struct tss
{
    uint32_t backlink;
    uint32_t* esp0;
    uint32_t ss0;
    uint32_t* esp1;
    uint32_t ss1;
    uint32_t* esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t (*eip)(void);
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;

    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;

    uint32_t trace;

    uint32_t io_base;
};

static struct tss tss;

// 更新tss中esp0 的值为 pthread 的0级线程
void update_tss_esp(struct task_struct* pthread){
    tss.esp0 = (uint32_t*)((uint32_t)pthread + PG_SIZE);
}

// 创建gdt描述符 p501
static struct gdt_desc make_gdt_desc(void* desc_addr, uint32_t limit, uint8_t attr_low, uint8_t attr_high){
    uint32_t desc_base = (uint32_t)desc_addr;
    struct gdt_desc desc;
    desc.limit_low_word = limit & 0xffff;
    desc.base_low_word = desc_base & 0xffff;
    desc.base_mid_byte = (desc_base & 0xff0000) >> 16;
    desc.attr_low_byte = attr_low;
    desc.limit_high_attr_high = (limit & 0xf0000) >> 16 | (uint8_t)attr_high;
    desc.base_high_byte = desc_base >> 24;
    return desc;
}

// 在gdt中创建tss并重新加载gdt
void tss_init(){
    put_str("tss_init start\n");
    
    uint32_t tss_size = sizeof tss;
    memset(&tss, 0, tss_size);
    tss.ss0 = SELECTOR_K_STACK;
    tss.io_base = tss_size; // 当IO位图的偏移地址>= TSS大小 -1时，表示没有IO位图

    // gdt 段基址为 0x900, 把tss放到第四个位置,也就是0x900+8*4=0x920的位置
    // 在gdt中添加 dpl为0的tss描述符
    struct gdt_desc* desc = 0xc0000000 + (GDT_BASE + 8*4);
    *(desc + 0) = make_gdt_desc(&tss, tss_size - 1, TSS_ATTR_LOW, TSS_ATTR_HIGH);
    // 在gdt中添加dpl为3的数据段和代码段描述符
    *(desc + 1) = make_gdt_desc(NULL, 0xfffff, GDT_CODE_ATTR_LOW_DPL3, GDT_ATTR_HIGH);
    *(desc + 2) = make_gdt_desc(NULL, 0xfffff, GDT_DATA_ATTR_LOW_DPL3, GDT_ATTR_HIGH);

    // gdt 16位的limit 32位的段基址
    uint64_t gdt_operand = (8 * 7 - 1) | ((uint64_t)(uint32_t)(0xc0000000 + GDT_BASE) << 16); // 7 个描述符大小
    asm volatile("lgdt %0"::"m"(gdt_operand));
    asm volatile("ltr %w0"::"r"(SELECTOR_TSS));

    put_str("tss_init and ltr done\n");
}
