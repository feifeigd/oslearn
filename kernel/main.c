#include <debug.h>
#include <init.h>
#include <print.h>

void k_thread_a(void* arg){
    char* para = arg;
    put_str("run thread:");
    put_str(para);
    put_str("\n");
}

int main(void){ // b 0x1500
    put_str("I am kernel\n");
    put_int(123);
    init_all();
    //asm volatile("sti"); // 为演示中断处理，在此临时开中断
    // ASSERT(1 == 2);
    void* addr = get_kernel_pages(3);   // 申请3个内存页, b 0x1546
    put_str("\n    get_kernel_page start vaddr is "); put_int((uint32_t)addr); put_str("\n");

    put_str("create thread\n");
    thread_start("k_thread_a", 31, k_thread_a, "argA");

    while(1);
    return 0;
}
