#pragma once

#include <stdint.h>

// -------------- GDT 描述符熟悉
#define DESC_G_4K   1
#define DESC_D_32   1
#define DESC_L      0   // 64位代码标记，此处标记为0
#define DESC_AVL    0   // cpu 不用此位，暂置为0
#define DESC_P      1

#define DESC_DPL_0  0
#define DESC_DPL_1  1
#define DESC_DPL_2  2
#define DESC_DPL_3  3

// 代码段和数据段属于存储段，tss和各种描述符属于系统段
// s=1表示存储段，s=0表示系统段
#define DESC_S_CODE 1
#define DESC_S_DATA DESC_S_CODE
#define DESC_S_SYS  0
#define DESC_TYPE_CODE  8   // x=1,c=0,r=0,a=0 代码段是可执行的，非依从的，不可读的，已访问为a清零
#define DESC_TYPE_DATA  2   // x=0,e=0,w=1,a=0 数据段是不可执行的，向上扩展的，可写的，已访问位a清零
#define DESC_TYPE_TSS   9   // B位为0，不忙

#define RPL1_0_0 0
#define RPL1_0_1 1
#define RPL1_0_2 2
#define RPL1_0_3 3

#define TI_GDT_2 0
#define TI_lDT_2 1
#define SELECTOR_K_CODE ((1 << 3) | (TI_GDT_2 << 2) | RPL1_0_0)
#define SELECTOR_K_DATA ((2 << 3) | (TI_GDT_2 << 2) | RPL1_0_0)
#define SELECTOR_K_STACK SELECTOR_K_DATA
#define SELECTOR_K_GS   ((3 << 3) | (TI_GDT_2 << 2) | RPL1_0_0)



// -------------- IDT 描述符属性
#define IDT_DESC_P      1
#define IDT_DESC_DPL0   0
#define IDT_DESC_DPL3   3
#define IDT_DESC_32_TYPE 0xE // 32位的门
#define IDT_DESC_16_TYPE 0x6 // 16位的门

#define IDT_DESC_ATTR_DPL0 ((IDT_DESC_P << 7)| (IDT_DESC_DPL0 << 5) | (IDT_DESC_32_TYPE))
#define IDT_DESC_ATTR_DPL3 ((IDT_DESC_P << 7)| (IDT_DESC_DPL3 << 5) | (IDT_DESC_32_TYPE))


#define NULL ((void*)0)
#define DIV_ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP))
#define bool int
#define true    1
#define false   0

#define PG_SIZE 4096
#define UNUSED __attribute__((unused))
