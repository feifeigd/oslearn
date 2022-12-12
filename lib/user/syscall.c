#include "syscall.h"

// 无参数的系统调用
#define _syscall0(NUMBER) ({\
    int retval;             \
    asm volatile("int $0x80":"=a"(retval):"a"(NUMBER):"memory");    \
    retval;                 \
})

uint32_t getpid(void){
    return _syscall0(SYS_GETPID);
}
