#pragma once
#include <global.h>

#define default_prip 31 // 默认优先级

#define USER_STACK3_VADDR (0xc000000 - PAGE1) // 留一页
#define USER_VADDR_START 0x8048000 // 装载地址

void start_process(void* filename_);
