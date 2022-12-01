#pragma once

#include <stdint.h>

// -------------- GDT 描述符熟悉
#define DESC_G_4K 1

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
