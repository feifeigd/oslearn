#pragma once

#include <stdint.h>
typedef void* intr_handler;

// 定义中断的两种状态
enum intr_status{
    INTR_OFF = 0,
    INTR_ON,
};

enum intr_status intr_get_status(void);
enum intr_status intr_set_status(enum intr_status status);
enum intr_status intr_enable(void);
enum intr_status intr_disable(void);
void register_handler(uint8_t vector_no, intr_handler function);

void idt_init(void);
