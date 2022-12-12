#pragma once

#include <print.h>

void console_init(void);

void console_put_char(uint8_t char_asci);
void console_put_int(uint32_t num);
void console_put_str(char const* str);
