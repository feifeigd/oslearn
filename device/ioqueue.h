#pragma once

#include <stdint.h>
#include <sync.h>

#define bufsize 64
// 环形队列
struct ioqueue
{
    struct lock lock;
    // 生产者
    struct task_struct* producer;

    // 消费者
    struct task_struct* consumer;
    char buf[bufsize];
    int32_t head;
    int32_t tail;
};

void ioqueue_init(struct ioqueue* ioq);
bool ioq_empty(struct ioqueue* ioq);
bool ioq_full(struct ioqueue* ioq);
char ioq_getchar(struct ioqueue* ioq);
void ioq_putchar(struct ioqueue* ioq, char byte);
