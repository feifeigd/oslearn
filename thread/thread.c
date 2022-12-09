
#include "thread.h"
#include <global.h>
#include <string.h>

// 由 kernel_thread 去执行 function(func_arg)
static void kernel_thread(thread_func* function, void* func_arg){
    function(func_arg);
}

// 初始化线程栈 thread_stack
// 将待执行的函数和参数放到thread_stack 相应的位置
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg)
{
    // 先预留中断使用栈的空间
    pthread->self_kstack -= sizeof(struct intr_stack);

    // 再留出线程栈空间
    pthread->self_kstack -= sizeof(struct thread_stack);
    struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    // 全部赋值 0
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}

// 初始化线程基本信息
void init_thread(struct task_struct* pthread, char* name, int prio){
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);
    pthread->status = TASK_RUNNING;
    pthread->priority = prio;
    // self_kstack 是线程自己栈内核态下使用的栈顶地址
    pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);
    pthread->stack_magic = 0x19851122; // 自定义魔数
}

// 创建一优先级为prio的线程，线程名为 name
// 线程所执行的函数时 function(func_arg)
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg){
    // pcb 都位于内核空间，包括用户进程的pcb也是内核空间
    struct task_struct* thread = get_kernel_pages(1);

    init_thread(thread, name, prio);
    asm volatile("mov %0, %%esp;pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret"::"g"(thread->self_kstack): "memory");
    return thread;
}
