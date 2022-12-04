#pragma once

#include <global.h>
#include <stdint.h>

struct bitmap{
    uint32_t btmp_bytes_len;
    uint8_t* bits;
};

void bitmap_init(struct bitmap* btmp);
bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx);

// 在位图中申请连续cnt个位，成功则返回其起始位下标，失败返回-1
int bitmap_scan(struct bitmap* btmp, uint32_t cnt);
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value);
