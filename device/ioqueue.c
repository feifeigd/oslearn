#include "ioqueue.h"
#include <debug.h>
#include <interrupt.h>
#include <sync.h>
#include <thread.h>

void ioqueue_init(struct ioqueue* ioq){
    lock_init(&ioq->lock);
    ioq->producer = ioq->consumer = NULL;
    ioq->head = ioq->tail = 0;
}

static int32_t next_pos(int32_t pos){
    return (pos + 1) % bufsize;
}

bool ioq_full(struct ioqueue* ioq){
    ASSERT(INTR_OFF == intr_get_status());
    return next_pos(ioq->head) == ioq->tail; 
}

bool ioq_empty(struct ioqueue* ioq){
    ASSERT(INTR_OFF == intr_get_status());
    return (ioq->head) == ioq->tail; 
}

static void ioq_wait(struct task_struct** waiter){
    ASSERT( waiter != NULL && *waiter == NULL);
    *waiter = running_thread();
    thread_block(TASK_BLOCKED);
}

static void wakeup(struct task_struct** waiter){
    ASSERT(*waiter);
    thread_unblock(*waiter);
    *waiter = NULL;
}

char ioq_getchar(struct ioqueue* ioq){
    ASSERT(INTR_OFF == intr_get_status());
    while (ioq_empty(ioq))
    {
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->consumer);
        lock_release(&ioq->lock);
    }
    
    char byte = ioq->buf[ioq->tail];
    ioq->tail = next_pos(ioq->tail);

    if(ioq->producer)
        wakeup(&ioq->producer);
    return byte;
}

void ioq_putchar(struct ioqueue* ioq, char byte){
    ASSERT(INTR_OFF == intr_get_status());
    while(ioq_full(ioq)){
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->producer);
        lock_release(&ioq->lock);
    }
    ioq->buf[ioq->head] = byte;
    ioq->head = next_pos(ioq->head);
    if (ioq->consumer)
    {
        wakeup(&ioq->consumer);
    }    
}
