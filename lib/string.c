#include "string.h"
#include <debug.h>
#include <global.h>

void memset(void* dst, uint8_t value, uint32_t size){
    ASSERT(dst);
    char* p = dst;
    while(size-- > 0)*p++ = value;
}

void memcpy(void* dst, const void* src, uint32_t size){
    ASSERT(dst);
    ASSERT(src);
    char* p_dst = dst;
    char* p_src = src;
    while(size-- > 0)*p_dst++ = *p_src++;
}

int memcmp(const void* a, const void* b, uint32_t size){
    ASSERT(a);ASSERT(b);
    char* pa = a;
    char* pb = b;
    while(size-- > 0){
        if(*pa > *pb)
            return 1;
        if(*pa++ < *pb++)
            return -1;
    }
    return 0;
}

char* strcpy(char* dst, const char* src){
    char* p_dst = dst;
    while(*src)*p_dst++ = *src++;
    return dst;
}

uint32_t strlen(const char* str){
    ASSERT(str);
    uint32_t count = 0;
    while(*str++)++count;
    return count;
}

int8_t strcmp(const char* a, const char* b){
    ASSERT(a);ASSERT(b);
    while(*a && *b){
        if(*a > *b)return 1;
        if(*a < *b)return -1;
        ++a;
        ++b;
    }
    if(*a)return 1;
    if(*b)return -1;
    return 0;
}

char* strchr(const char* str, const uint8_t ch){
    ASSERT(str);
    while(*str){
        if(*str++ == ch)
            return --str;
    }
    return NULL;
}

char* strrchr(const char* str, const uint8_t ch){
    ASSERT(str);
    const char* last_char = NULL;
    while(*str){
        if(*str == ch)
            last_char = str;
        ++str;
    }
    return (char*)last_char;
}

char* strcat(char* dst, const char* src){
    ASSERT(dst);
    ASSERT(src);
    char* str = dst;

    while(*str++);
    --str;
    while((*str++ = *src++));

    return dst;
}

uint32_t strchrs(const char* str, uint8_t ch){
    ASSERT(str);
    uint32_t ch_cnt = 0;
    char const* p = str;
    while(*p){
        if(*p == ch)
            ++ch_cnt;
        ++p;
    }
    
    return ch_cnt;
}
