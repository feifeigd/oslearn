#include "console.h"

#include <print.h>
#include <sync.h>

static struct lock console_lock;

void console_init(void){
    lock_init(&console_lock);    
}

void console_acquire(){
    lock_acquire(&console_lock);
}

void console_release(){
    lock_release(&console_lock);
}

void console_put_str(char const* str){
    console_acquire();
    put_str(str);
    console_release();
}

void console_put_char(uint8_t char_asci){
    console_acquire();
    put_char(char_asci);
    console_release();
}

void console_put_int(uint32_t num){
    console_acquire();
    put_int(num);
    console_release();
}

