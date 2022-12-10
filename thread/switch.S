
[bits 32]
section .text

; switch_to(cur, next);
global switch_to

switch_to:
    ; 栈中此时是返回地址
    push esi
    push edi
    push ebx
    push ebp

    mov eax, [esp + 20]; 得到栈中的参数 cur,
    mov [eax], esp; 保存栈顶指针 esp. task_struct 的 self_kstack 字段，

    ; 以上是备份当前线程的环境，下面是恢复下一个线程的环境
    
    mov eax, [esp + 24]; 得到栈中的next
    mov esp, [eax]; pcb 的第一个成员是 self_kstack, 它用来记录0级栈顶指针，被换上 cpu时用了恢复0级栈，0级栈中保存了进程或线程所有信息，包括3级指针

    pop ebp
    pop ebx
    pop edi
    pop esi

    ret

