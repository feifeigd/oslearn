#include "list.h"
#include <interrupt.h>

void list_init(struct list*){
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
}

void list_insert_before(struct list_elem* before, struct list_elem* elem){
    enum intr_status old_status = intr_disable();

    // 将 before 前驱元素的后继元素更新为elem，暂时使before脱离链表
    before->prev->next = elem;

    elem->prev = before->prev;
    elem->next = before;

    before->prev = elem;

    intr_set_status(old_status);
}

// 添加元素到队首
void list_push(struct list* plist, struct list_elem* elem){
    list_insert_before(plist->head.next, elem);
}

// 添加元素到队尾
void list_append(struct list* plist, struct list_elem* elem){
    list_insert_before(&list->tail, elem);
}

void list_iterate(struct list* plist);

// 脱离链表
void list_remove(struct list_elem* pelem){
    enum intr_status old_status = intr_disable();

    pelem->prev->next = pelem->next;
    pelem->next->prev = pelem->prev;

    intr_set_status(old_status);
}

struct list_elem* list_pop(struct list* plist){
    struct list_elem* elem = plist->head.next;
    list_remove(elem);
    return elem;
}

bool list_empty(struct list* plist){
    return plist->head.next == &plist->tail;
}

uint32_t list_len(struct list* plist){
    struct list_elem* elem = plist->head.next;
    uint32_t length = 0;
    while (elem != &plist->tail)
    {
        ++length;
        elem = elem->next;
    }
    
    return length;
}

// 返回符合条件的节点
struct list_elem* list_traversal(struct list* plist, function func, int arg){
    struct list_elem* elem = plist->head.next;
    if(list_empty(plist))
        return NULL;
    while (elem != &plist->tail)
    {
        if(func(elem, arg))
            return elem;
        elem = elem->next;
    }
    return NULL;
}

// 节点是否中链表里
bool elem_find(struct list* plist, struct list_elem* obj_elem){
    struct list_elem* elem = plist->head.next;
    while (elem != &plist->tail)
    {
        if(elem == obj_elem)
            return true;
        elem = elem->next;
    }
    return false;
}
