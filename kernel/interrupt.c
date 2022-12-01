#include "global.h"
#include "interrupt.h"
#include <io.h>
#include <print.h>

#define PIC_M_CTRL 0x20 // 这里用的可编程中断控制器是8259A，主片的控制端口是0x20
#define PIC_M_DATA 0x21 // 主片的数据端口是
#define PIC_S_CTRL 0xa0 // 从片的控制端口
#define PIC_S_DATA 0xa1 // 从片的数据端口

#define IDT_DESC_CNT 0x81   // 目前总共支持的中断数

// 8字节
struct gate_desc
{
    uint16_t func_offset_low_word;// 中断处理程序在目标代码段内的偏移量的低16位
    uint16_t selector;// 中断处理程序目标代码段描述符选择子
    uint8_t dcount; // 此项为双字计数字段，是门描述符中的第4字节，此项固定值，不用考虑
    uint8_t attribute;// P | DPL | S | TYPE
    uint16_t func_offset_high_word;// 中断处理程序在目标代码段内的偏移量的高16位
};

static struct gate_desc idt[IDT_DESC_CNT];

// 定义中断处理程序数组
// 在kernel.S 中定义的 intrXXentry 只是中断处理程序的入口
// 最终调用的是 ide_table中的处理程序
extern intr_handler intr_entry_table[IDT_DESC_CNT]; // 声明引用定义在kernel.S 中的中断处理函数入口数组

// 初始化可编程中断控制器8259A
static void pic_init(void){
    // 初始化主片
    outb(PIC_M_CTRL, 0x11); // ICW1: 边沿触发，级联8259A，需要 ICW4
    outb(PIC_M_DATA, 0x20); // ICW2: 起始中断向量号为0x20,也就是IR[0~7]为 0x20 ~ 0x27
    outb(PIC_M_DATA, 0X04); // ICW3: IR2接从片
    outb(PIC_M_DATA, 0x01); // ICW4: 8086模式，正常EOI

    // 初始化从片
    outb(PIC_S_CTRL, 0x11); // ICW1: 边沿触发，级联8259A，需要ICW4
    outb(PIC_S_DATA, 0x28); // ICW2: 起始中断向量号为0x28,也就是IR[8~15]为 0x28~0x2f.
    outb(PIC_S_DATA, 0x02); // ICW3: 设置从片连接到主片的 IR2引脚
    outb(PIC_S_DATA, 0x01); // ICW4: 8086模式，正常EOI

    // IRQ2用于级联从片，必须打开，否则无法响应从片上的中断
    // 主片上打开的中断有IRQ0的时钟，IRQ1的键盘和级联从片的IRQ2，其他全部关闭
    outb(PIC_M_DATA, 0xf8);

    // 打开从片上的IRQ4，次引脚接收硬盘控制器的中断
    outb(PIC_S_DATA, 0xbf);

    put_str("pic_init done\n");
}

// 创建中断门描述符
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function){
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
    p_gdesc->func_offset_high_word = ((uint32_t)function >> 16) & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
}

// 初始化中断描述符表
static void idt_desc_init(void){
    int i, lastindex = IDT_DESC_CNT - 1;
    for(i = 0; i < IDT_DESC_CNT; ++i){
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    put_str("idt_desc_init done\n");
}

// 完成有关中断的所有初始化工作
void idt_init(void){
    put_str("idt_init start\n");
    idt_desc_init(); // 初始化中断描述符表
    pic_init(); // 初始化 8259A

    // 加载 idt
    uint64_t idt_operand = (sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16));
    asm volatile("lidt %0"::"m"(idt_operand));
    put_str("idt_init done\n");
}
