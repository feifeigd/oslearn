#include "keyboard.h"

#include <global.h> // bool
#include <io.h>
#include <stdio.h>

#define KBD_BUF_PORT 0x60   // 键盘buffer寄存器端口号

#define esc         '\033'
#define backspace   '\b'
#define tab         '\t'
#define enter       '\r'
#define delete      '\177'

// 以下是不可见字符,一律定义为0
#define char_invisible  0
#define ctrl_l_char     char_invisible
#define ctrl_r_char     char_invisible
#define shift_l_char    char_invisible
#define shift_r_char    char_invisible
#define alt_l_char      char_invisible
#define alt_r_char      char_invisible
#define caps_lock_char  char_invisible

// 定义控制字符的通码(make)和断码(break)
#define shift_l_make    0x2a
#define shift_r_make    0x36
#define alt_l_make      0x38
#define alt_r_make      0xe038
#define alt_r_break     0xe0b8
#define ctrl_l_make     0x1d
#define ctrl_r_make     0xe01d
#define ctrl_r_break    0xe09d
#define caps_lock_make  0x3a

// 定义以下变量记录相应键是否按下的状态
// ext_scancode 用于记录makecode是否以0xe0开头
static bool ctrl_status, shift_status, alt_status, caps_lock_status, ext_scancode;

// 以通码make_code 为索引的二维数组
static char keymap[][2] = {
    // 扫描码未与shift组合
    {0, 0},// 0x00
    {esc, esc},// 0x01
};

// 键盘中断处理程序
static void intr_keyboard_handler(void){
    uint8_t scancode = inb(KBD_BUF_PORT);
    put_int(scancode);
}

void keyboard_init(void){
    put_str("keyboard init start\n");

    register_handler(0x21, intr_keyboard_handler);

    put_str("keyboard init end\n");
}
