#pragma once

#include <stdint.h>

// 系统调用子功能号
enum SYSCALL_NR{
    SYS_GETPID,
};

// 返回当前任务的pid
uint32_t getpid(void);
