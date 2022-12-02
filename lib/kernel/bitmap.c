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

int bitmap_scan(struct bitmap* btmp, uint32_t cnt);
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value){
    ASSERT(0 == value || 1 == value);
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd = bit_idx % 8;
    if(value)
        btmp->bits[byte_idx] |= 1 << bit_odd;
    else
        btmp->bits[byte_idx] &= ~(1 << bit_odd);
}
