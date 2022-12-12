#include "sync.h"
#include <debug.h>
#include <interrupt.h>
#include <thread.h>

void sema_init(struct semaphore* psema, uint8_t value){
    psema->value = value;
    list_init(&psema->waiters);
}

void lock_init(struct lock* plock){
    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    sema_init(&plock->semaphore, 1);
}

// 信号量down操作
void sema_down(struct semaphore* psema){
    // 关中断来保证原子操作
    enum intr_status old_status = intr_disable();
    while (0 == psema->value)   // 已经被别人持有
    {
        ASSERT(!elem_find(&psema->waiters, &running_thread()->general_tag));
        // 当前线程不应该已在信号量的waiters队列中
        if (elem_find(&psema->waiters, &running_thread()->general_tag))
        {
            PANIC("sema_down: thread blocked has been in waiters_listn");
        }
        // 加入等待队列，然后阻塞自己
        list_append(&psema->waiters, &running_thread()->general_tag);
        thread_block(TASK_BLOCKED);
    }
    
    --psema->value;// 获得锁
    ASSERT(0 == psema->value);
    intr_set_status(old_status);
}

// 信号量up操作
void sema_up(struct semaphore* psema){
    enum intr_status old_status = intr_disable();
    ASSERT(0 == psema->value);
    if (!list_empty(&psema->waiters))// 唤醒一个线程
    {
        struct task_struct* thread_blocked = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));
        thread_unblock(thread_blocked);
    }
    ++psema->value;
    ASSERT(1 == psema->value);
    intr_set_status(old_status);
}

// 获取锁
void lock_acquire(struct lock* plock){
    if (running_thread() == plock->holder)
    {
        ++plock->holder_repeat_nr;
    }else{
        sema_down(&plock->semaphore);   // 对信号量P操作，原子操作
        plock->holder = running_thread();
        ASSERT(0 == plock->holder_repeat_nr);
        plock->holder_repeat_nr = 1; // 第一次申请到锁
    }    
}

// 释放锁
void lock_release(struct lock* plock){
    ASSERT(running_thread() == plock->holder);
    if(plock->holder_repeat_nr > 0){
        --plock->holder_repeat_nr;
        return;
    }
    // 最后一次释放
    ASSERT(1 == plock->holder_repeat_nr);

    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    sema_up(&plock->semaphore);// 信号量的V操作，也是原子操作
}
