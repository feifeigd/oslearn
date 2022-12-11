#pragma once

#include <stdint.h>

void put_char(uint8_t char_asci);
void put_str(char const* str);
void put_int(uint32_t num);

void set_cursor(int pos);
