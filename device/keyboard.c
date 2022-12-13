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
// 第一套键盘扫描码
static char keymap[][2] = {
    // 扫描码未与shift组合
    {0, 0},     // 0x00
    {esc, esc}, // 0x01
    {'1', '!'}, // 0x02
    {'2', '@'},     // 0x03
    {'3', '#'},     // 0x04
    {'4', '$'},     // 0x05
    {'5', '%'},     // 0x06
    {'6', '^'},     // 0x07
    {'7', '&'},     // 0x08
    {'8', '*'},     // 0x09
    {'9', '('},     // 0x0A
    {'0', ')'},     // 0x0B
    {'-', '_'},     // 0x0C
    {'=', '+'},     // 0x0D
    {backspace, backspace},     // 0x0E
    {tab, tab},     // 0x0F

    {'q', 'Q'},     // 0x10
    {'w', 'W'},     // 0x11
    {'e', 'E'},     // 0x12
    {'r', 'R'},     // 0x13
    {'t', 'T'},     // 0x14
    {'y', 'Y'},     // 0x15
    {'u', 'U'},     // 0x16
    {'i', 'I'},     // 0x17
    {'o', 'O'},     // 0x18
    {'p', 'P'},     // 0x19
    {'[', '{'},     // 0x1A
    {']', '}'},     // 0x1B
    {enter, enter},     // 0x1C
    {ctrl_l_char, ctrl_l_char},     // 0x1D
    {'a', 'A'},     // 0x1E
    {'s', 'S'},     // 0x1F

    {'d', 'D'},     // 0x20
    {'f', 'F'},     // 0x21
    {'g', 'G'},     // 0x22
    {'h', 'H'},     // 0x23
    {'j', 'J'},     // 0x24
    {'k', 'K'},     // 0x25
    {'l', 'L'},     // 0x26
    {';', ':'},     // 0x27
    {'\'', '\"'},     // 0x28
    {'`', '~'},     // 0x29
    {shift_l_char, shift_l_char},     // 0x2A
    {'\\', '|'},     // 0x2B
    {'z', 'Z'},     // 0x2C
    {'x', 'X'},     // 0x2D
    {'c', 'C'},     // 0x2E
    {'v', 'V'},     // 0x2F

    {'b', 'B'},     // 0x30
    {'n', 'N'},     // 0x31
    {'m', 'M'},     // 0x32
    {',', '<'},     // 0x33
    {'.', '>'},     // 0x34
    {'/', '?'},     // 0x35
    {shift_r_char, shift_r_char},     // 0x36
    {'*', '*'},     // 0x37
    {alt_l_char, alt_l_char},     // 0x38
    {' ', ' '},     // 0x39
    {caps_lock_char, caps_lock_char},     // 0x3A
    /*/ 其他暂时不管
    {']', '}'},     // 0x3B
    {enter, enter},     // 0x3C
    {ctrl_l_char, ctrl_l_char},     // 0x3D
    {'a', 'A'},     // 0x3E
    {'s', 'S'},     // 0x3F*/
};

// 键盘中断处理程序
static void intr_keyboard_handler(void){
    bool ctrl_down_last = ctrl_status;
    bool shift_down_last = shift_status;
    bool caps_lock_last = caps_lock_status;

    bool break_code;
    uint16_t scancode = inb(KBD_BUF_PORT);
    // 若扫描码是e0开头的，表示此键的按下将产生多个扫描码
    // 所以马上结束此次中断处理函数，等待下一个扫描码进来
    if(0xe0 == scancode){
        ext_scancode = true; // 打开 e0 标记
        return;
    }
    // 合并扫描码
    if (ext_scancode)
    {
        scancode |= 0xe000;
        ext_scancode = false;
    }

    break_code = 0x80 & scancode; 
    if (break_code) // 按键弹起
    {
        // 由于ctrl_r 和 alt_r 的 make_code 和 break_code 都是两字节，所以可用下面的方法取make_code,多字节的扫描码暂不处理
        uint16_t make_code = scancode & 0xff7f;
        // 得到其 make_code(按键按下时产生的扫描码)

        // 若是任意以下三个键弹起了，将状态置为false
        if(ctrl_l_make == make_code || ctrl_r_make == make_code)
            ctrl_status = false;
        else if(shift_l_make == make_code || shift_r_make == make_code)
            shift_status = false;
        else if(alt_l_make == make_code || alt_r_make == make_code)
            alt_status = false;

        return;
    }else if( (scancode > 0 && scancode < 0x3B) || alt_r_make == scancode || ctrl_r_make == scancode){  // 若为通码，只处理数组中定义的键，以及alt_right和ctrl键，全是make_code
        bool shift = false;
        if (scancode < 0x0e // '0' ~ '9' '-' '='
            || scancode == 0x1a || scancode == 0x1b // '[' ']'
            || scancode == 0x2b || scancode == 0x27 || scancode == 0x28 || scancode == 0x29 // '\\' ';' '\'' '`'
            || scancode == 0x33 || scancode == 0x34|| scancode == 0x35) // ',' '.' '/'
        {
            if(shift_down_last)
                shift = true;
        }else{// 默认为字母键
            if(shift_down_last && caps_lock_last)// 两个键同时按下
                shift = false; 
            else if(shift_down_last || caps_lock_last)
                shift = true;
            else
                shift = false;
        }
        
        uint8_t index = scancode &= 0xff;
        char cur_char = keymap[index][shift];
        // 只处理ASCII码不为零的键
        if(cur_char){
            put_char(cur_char);
            return;
        }
        // 记录本次是否按下了下面几类控制键,供下次键入时判断组合键
        
        if(ctrl_l_make == scancode || ctrl_r_make == scancode)
            ctrl_status = true;
        else if(shift_l_make == scancode || shift_r_make == scancode)
            shift_status = true;
        else if(alt_l_make == scancode || alt_r_make == scancode)
            alt_status = true;
        else 
            caps_lock_status = !caps_lock_status;   // 大写键 反转
    }else{  
        put_str("unknown key\n");
    }    
    
    // put_int(scancode);
}

void keyboard_init(void){
    put_str("keyboard init start\n");

    register_handler(0x21, intr_keyboard_handler);

    put_str("keyboard init end\n");
}
