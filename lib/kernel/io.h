#pragma once

// 机器模式
// b 输出寄存器QImode名称,即寄存器中的最低8位:[a-d]l
// w 输出寄存器HImode名称，即寄存器中2个字节的部分，如[a-d]x
// 
// HImode
//      Half-Integer 模式，表示一个两字节的整数
// QImode
//      Quarter-Integer 模式，表示一个一字节的整数

#include <stdint.h>

// 向端口port写入一个字节
static inline void outb(uint16_t port, uint8_t data){
    // a 表示用寄存器al或ax或eax，对端口指定N表示0~255，d表示用dx存储端口号
    // %b0表示对应al,%w1表示对应dx
    asm volatile("outb %b0, %w1"::"a"(data), "Nd"(port));
}
