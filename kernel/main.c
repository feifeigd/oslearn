#include "debug.h"
#include "global.h"
#include "init.h"
#include "interrupt.h"
#include "keyboard.h"
#include "memory.h"
#include <print.h>
#include <thread.h>

static void k_thread_a(void* arg);
static void k_thread_b(void* arg);

int main(void){ // b 0x1500
    put_str("I am kernel\n");
    put_int(123);
    init_all();
    
    // ASSERT(1 == 2);
    void* addr = get_kernel_pages(3);   // 申请3个内存页, b 0x1546
    put_str("\n    get_kernel_page start vaddr is "); put_int((uint32_t)addr); put_str("\n");
    // intr_enable(); // 打开中断，使时钟中断起作用
    put_str("create thread\n");
    char* arg = "argA";
    thread_start("k_thread_a", 31, k_thread_a, arg);
    thread_start("k_thread_b", 30, k_thread_b, "argB");

    intr_enable();

    while(1){
        //console_put_str("Main ");
    }
    
    return 0;
}

static void k_thread_a(void* arg){
    
    char* para = arg;
    while (true)
    {
        enum intr_status old_status = intr_disable();
        if(!ioq_empty(&kbd_buf)){
            console_put_str(para);
            char byte = ioq_getchar(&kbd_buf);
            console_put_char(byte);
        }     
        intr_set_status(old_status);
        // return;
    }    
}

static void k_thread_b(void* arg){
    
    char* para = arg;
    while (true)
    {
        enum intr_status old_status = intr_disable();
        if(!ioq_empty(&kbd_buf)){
            console_put_str(para);
            char byte = ioq_getchar(&kbd_buf);
            console_put_char(byte);
        }     
        intr_set_status(old_status);
        // return;
    }    
}
