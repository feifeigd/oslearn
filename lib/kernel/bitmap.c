#include "bitmap.h"
#include <debug.h>
#include <string.h>

void bitmap_init(struct bitmap* btmp){
    memset(btmp->bits, 0, btmp->btmp_bytes_len);
}

bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx){
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd = bit_idx % 8;
    return btmp->bits[byte_idx] & (1 << bit_odd);
}

int bitmap_scan(struct bitmap* btmp, uint32_t cnt){
    uint32_t idx_byte = 0; // 用于记录空闲位所在的字节
    while ((0xff == btmp->bits[idx_byte]) && (idx_byte < btmp->btmp_bytes_len))
        ++idx_byte;
    ASSERT(idx_byte < btmp->btmp_bytes_len);
    if(idx_byte == btmp->btmp_bytes_len)
        return -1;
    int idx_bit = 0;
    while ((uint8_t)(1 << idx_bit) & btmp->bits[idx_byte])
        ++idx_bit;
    int bit_idx_start = idx_byte * 8 + idx_bit;
    if(1 == cnt)
        return bit_idx_start;
    uint32_t bit_left = btmp->btmp_bytes_len * 8 - bit_idx_start;
    uint32_t next_bit = bit_idx_start + 1;
    uint32_t count = 1; // 用于记录找到的空闲位的个数
    bit_idx_start = -1;
    while (bit_left-- > 0)
    {
        if(!bitmap_scan_test(btmp, next_bit))
            ++count;
        else
            count = 0;
        if(cnt == count)
            return next_bit - cnt + 1;
        ++next_bit;
    }
    
    return bit_idx_start;
}

void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value){
    ASSERT(0 == value || 1 == value);
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd = bit_idx % 8;
    if(value)
        btmp->bits[byte_idx] |= 1 << bit_odd;
    else
        btmp->bits[byte_idx] &= ~(1 << bit_odd);
}
