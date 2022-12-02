#pragma once

#include "stdint.h"

void memset(void* dst, uint8_t value, uint32_t size);
void memcpy(void* dst, const void* src, uint32_t size);
int memcmp(const void* a, const void* b, uint32_t size);
char* strcpy(char* dst, const char* src);
uint32_t strlen(const char* str);
int8_t strcmp(const char* a, const char* b);
char* strchr(const char* str, const uint8_t ch);

// 从后往前查找字符串str中首次出现字符ch的地址
char* strrchr(const char* str, const uint8_t ch);
char* strcat(char* dst, const char* src);

// 查找ch出现的次数
uint32_t strchrs(const char* str, uint8_t ch);
