#include "global.h"
#include "interrupt.h"
#include <io.h>
#include <print.h>

#define PIC_M_CTRL 0x20 // 这里用的可编程中断控制器是8259A，主片的控制端口是0x20
#define PIC_M_DATA 0x21 // 主片的数据端口是
#define PIC_S_CTRL 0xa0 // 从片的控制端口
#define PIC_S_DATA 0xa1 // 从片的数据端口

#define IDT_DESC_CNT 0x81   // 目前总共支持的中断数

#define EFLAGS_IF 0x00000200    // eflags寄存器的if位为1
#define GET_EFLAGS(EFLAG_VAR) asm volatile("pushfl; popl %0": "=g"(EFLAG_VAR))

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

char* intr_name[IDT_DESC_CNT];  // 用于保存异常的名字
intr_handler idt_table[IDT_DESC_CNT]; // 定义中断处理程序数组，在kernel.S 中定义的intXXentry 只是中断处理程序的入口，最终调用的是ide_table中的处理程序

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

// 通用的中断处理函数,一般用在异常出现时的处理
static void general_intr_handler(uint8_t vec_nr){
    if (0x27 == vec_nr || 0x2f == vec_nr)   // 0x2f 是从片8259A上的最后一个irq引脚，保留
        return; // IRQ7 和 IRQ15 会产生伪中断(spurious interrupt), 无须处理
    // 将光标置为0，从屏幕左上角清出一片打印异常信息的区域，方便阅读
    
    int cursor_pos = 0;
    while(cursor_pos < 320){    // 4行
        put_char(' ');
        ++cursor_pos;
    }
    set_cursor(0);
    put_str("!!!!!! exception message begin !!!!!!\n");
    set_cursor(88); // 第二行第8个字符
    put_str(intr_name[vec_nr]);
    if (14 == vec_nr)   // PageFault
    {
        int page_fault_vaddr = 0;
        asm("movl %%cr2, %0":"=r"(page_fault_vaddr));
        put_str("\npage fault addr is ");put_int(page_fault_vaddr);
    }
    put_str("!!!!!! exception message end !!!!!!\n");
    put_str("int vector: 0x");
    put_int(vec_nr);
    put_char('\n');    
    while(true); // 
}

// 完成一般中断处理函数注册及异常名称注册
static void exception_init(void){
    int i;
    for(i = 0; i < IDT_DESC_CNT; ++i){
        // idt_table 数组中的函数是在进入中断后根据中断向量号调用的
        // 见 kernel/kernel.S 中的 call [idt_table + %1 * 4]
        idt_table[i] = general_intr_handler;    // 默认
        intr_name[i] = "unknown"; // 先统一赋值
    }
    intr_name[0] = "#DE Divide Error";
    intr_name[1] = "#DB Debug Exception";
    intr_name[2] = "NMI Interrupt";
    intr_name[3] = "#BP Breakpoint Exception";
    intr_name[4] = "#OF Overflow Exception";
    intr_name[5] = "#BR BOUND Range Exceeded Exception";
    intr_name[6] = "#UD Invalid Opcode Exception";
    intr_name[7] = "#NM Device Not Avaliable Exception";
    intr_name[8] = "#DF Double Fault Exception";
    intr_name[9] = "Coprocessor Segment Overrun";
    intr_name[10] = "#TS Invalid TSS Exception";
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception";
    intr_name[14] = "#PF Page-Fault Exception";
    // intr_name[15] = "intel 保留项，未使用";
    intr_name[16] = "#MF x87 FPU Floating-Point Error";
    intr_name[17] = "#AC Alignment Check Exception";
    intr_name[18] = "#MC Machine-Check Exception";
    intr_name[19] = "#XF SIMD Floating-Point Exception";
}

// 完成有关中断的所有初始化工作
void idt_init(void){
    put_str("idt_init start\n");
    idt_desc_init(); // 初始化中断描述符表
    exception_init(); // 异常名初始化并注册的中断处理函数
    pic_init(); // 初始化 8259A

    // 加载 idt
    uint64_t idt_operand = (sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16));
    asm volatile("lidt %0"::"m"(idt_operand));
    put_str("idt_init done\n");
}

enum intr_status intr_get_status(void){
    uint32_t eflags = 0;
    GET_EFLAGS(eflags);
    return (EFLAGS_IF & eflags) ? INTR_ON : INTR_OFF;
}

enum intr_status intr_set_status(enum intr_status status){
    return status & INTR_ON ? intr_enable() : intr_disable();
}

enum intr_status intr_enable(void){
    enum intr_status old_status;
    if(INTR_ON == intr_get_status()){
        old_status = INTR_ON;
        return old_status;
    }
    old_status = INTR_OFF;
    asm volatile("sti"); // 开中断，sti指令将IF位置为1
    return old_status;
}

enum intr_status intr_disable(void){
    enum intr_status old_status;
    if(INTR_ON == intr_get_status()){
        old_status = INTR_ON;
        asm volatile("cli":::"memory");
        return old_status;
    }
    old_status = INTR_OFF;
    return old_status;
}

void register_handler(uint8_t vector_no, intr_handler function){
    idt_table[vector_no] = function;
}
