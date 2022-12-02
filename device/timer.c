#include "timer.h"
#include <io.h>
#include <print.h>

#define IRQ0_FREQUENCY      100     // 一秒钟 100次时钟中断
#define INPUT_FREQUENCY     1193180 // 计数器工作频率
#define COUNTER0_VALUE      INPUT_FREQUENCY / IRQ0_FREQUENCY
#define CONTRER0_PORT       0x40    // 计数器0
#define COUNTER0_NO         0
#define COUNTER_MODE        2
#define READ_WRITE_LATCH    3
#define PIT_CONTROL_PORT    0x43

static void frequency_set(uint8_t counter_port, uint8_t counter_no, uint8_t rwl, uint8_t counter_mode, uint16_t counter_value){
    outb(PIT_CONTROL_PORT, (uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1));
    // 先写入counter_value的低8位
    outb(counter_port, (uint8_t)counter_value);
    // 高8位    
    outb(counter_port, (uint8_t)(counter_value >> 8));
}

// 初始化 PIT 8253
void timer_init(void){
    put_str("timer_init start\n");
    // 设置8253的定时周期，也就是发中断的周期
    frequency_set(CONTRER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER_MODE, COUNTER0_VALUE);
    put_str("timer_init done\n");
}
