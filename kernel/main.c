#include <init.h>
#include <print.h>

int main(void){
    put_str("I am kernel\n");
    put_int(123);
    init_all();
    //asm volatile("sti"); // 为演示中断处理，在此临时开中断
    while(1);
    return 0;
}
