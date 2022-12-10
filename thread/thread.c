
#include "thread.h"
#include <debug.h>
#include <global.h>
#include <interrupt.h>
#include <string.h>

struct task_struct* main_thread;
struct list thread_ready_list;
struct list thread_all_list;
static struct list_elem* thread_tag; // 用于保存队列中的线程结点
extern void switch_to(struct task_struct* cur, struct task_struct* next);

// 由 kernel_thread 去执行 function(func_arg)
static void kernel_thread(thread_func* function, void* func_arg){
    // 开中断，避免后面的时钟中断被屏蔽，而无法调度其他线程
    intr_enable();

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
    // 全部赋值 0,因为线程尚未执行
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}

// 初始化线程基本信息
void init_thread(struct task_struct* pthread, char* name, int prio){
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);

    pthread->status = pthread == main_thread ? TASK_RUNNING : TASK_READY;   // 由于把main函数也封装成一个线程，并且它一直是运行的

    pthread->priority = pthread->ticks = prio;
    pthread->elapsed_ticks = 0;
    pthread->pgdir = NULL;
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
    thread_create(thread, function, func_arg);

    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    // 加入就绪队列
    list_append(&thread_ready_list, &thread->general_tag);

    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    // 加入全部线程队列
    list_append(&thread_all_list, &thread->all_list_tag);

    // asm volatile("movl %0, %%esp;pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret"::"g"(thread->self_kstack): "memory");
    return thread;
}

// 获取当前线程pcb指针
struct task_struct* running_thread(){
    uint32_t esp;
    asm ("mov %%esp, %0":"=g"(esp));
    // 取esp整数部分，即pcb起始地址
    return (struct task_struct*)(esp & 0xfffff000);
}

// 将kernel中的main函数完善为主线程
static void make_main_thread(void){
    // 因为main线程早已运行
    // 咱们中loader.S 中进入内核时的 mov esp, 0xc009f000
    // 就是为其保留pcb的，因此pcb地址为 0xc009e000
    // 不需要通过get_kernel_page另分配一页
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);

    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

// 实现任务调度
void schedule(){
    ASSERT(INTR_OFF == intr_get_status());
    struct task_struct* cur = running_thread();
    if(TASK_RUNNING == cur->status){
        // 若此线程只是cpu时间片到了，将其加入到就绪队列
        ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
        list_append(&thread_ready_list, &cur->general_tag);
        cur->ticks = cur->priority;
        cur->status = TASK_READY;
    }else{

    }

    ASSERT(!list_empty(&thread_ready_list));    // 就绪队列为空，可能要做另外的逻辑
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;
    put_str("switch to :"); put_int((int)next); put_str("\n");
    switch_to(cur, next);
}

// 初始化线程环境
void thread_init(void){
    put_str("thread_init start\n");

    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    
    // 将当前main函数创建为线程
    make_main_thread();

    put_str("thread_init end\n");
}
