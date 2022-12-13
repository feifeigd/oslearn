
[bits 32]

%define ERROR_CODE 	nop
%define ZERO 		push 0
%define EOI 		0x20
extern put_str; 
extern idt_table;

section .data
intr_str db "interrupt occour!", 0xa, 0

global intr_entry_table

%macro VECTOR 2
section .text
intr%1entry:	; 每个中断处理程序都要压入中断向量号，所以一个中断类型一个中断处理程序
	%2; 中断若有错误码，会压在 eip 后面
	; 保存上下文环境
	push ds;
	push es;
	push fs;
	push gs;
	pushad;

	;push intr_str;
	;call put_str;
	;add esp, 4; 跳过参数

	; 如果是从片上进入的中断，除了往从片上发送EOI外，还要往主片上发送 EOI;
	mov al, EOI;  中断结束命令EOI，手动结束中断
	out 0xa0, al; 往从片发送, OCW2
	out 0x20, al; 往主片发送, 为了让8259A接受新的中断，必须要让8259A知道当前中断处理程序已经执行完成

	push %1;
	call [idt_table + %1 * 4];	// 调C实现的中断处理函数

	jmp intr_exit
	
section .data
	dd intr%1entry; 存储各个中断入口程序的地址，形成intr_entry_table数组
%endmacro

section .text
global intr_exit
intr_exit:
	; 以下是恢复上下文环境
	add esp, 4; 跳过终端号

	popad
	pop gs;
	pop fs;
	pop es;
	pop ds
	add esp, 4; 跨过 error_code
	;hlt; debug
	iret; 从中断返回，32 位下等同指令 iretd;

section .data
intr_entry_table:
	VECTOR 0x00, ZERO
	VECTOR 0x01, ZERO
	VECTOR 0x02, ZERO
	VECTOR 0x03, ZERO
	VECTOR 0x04, ZERO
	VECTOR 0x05, ZERO
	VECTOR 0x06, ZERO
	VECTOR 0x07, ZERO
	VECTOR 0x08, ERROR_CODE
	VECTOR 0x09, ZERO
	VECTOR 0x0A, ERROR_CODE
	VECTOR 0x0B, ERROR_CODE
	VECTOR 0x0C, ZERO
	VECTOR 0x0D, ERROR_CODE
	VECTOR 0x0E, ERROR_CODE
	VECTOR 0x0F, ZERO
	
	VECTOR 0x10, ZERO
	VECTOR 0x11, ERROR_CODE
	VECTOR 0x12, ZERO
	VECTOR 0x13, ZERO
	VECTOR 0x14, ZERO
	VECTOR 0x15, ZERO
	VECTOR 0x16, ZERO
	VECTOR 0x17, ZERO
	VECTOR 0x18, ERROR_CODE
	VECTOR 0x19, ZERO
	VECTOR 0x1A, ERROR_CODE
	VECTOR 0x1B, ERROR_CODE
	VECTOR 0x1C, ZERO
	VECTOR 0x1D, ERROR_CODE
	VECTOR 0x1E, ERROR_CODE
	VECTOR 0x1F, ZERO

	VECTOR 0x20, ZERO; 时钟中断对应的入口
	VECTOR 0x21, ZERO; 键盘中断对应的入口
	VECTOR 0x22, ZERO; 级联用
	VECTOR 0x23, ZERO; 串口2对应的入口
	VECTOR 0x24, ZERO; 串口1对应的入口
	VECTOR 0x25, ZERO; 并口2对应的入口
	VECTOR 0x26, ZERO; 软盘对应的入口
	VECTOR 0x27, ZERO; 并口1对应的入口
	VECTOR 0x28, ZERO; 实时时钟对应的入口
	VECTOR 0x29, ZERO; 重定向
	VECTOR 0x2A, ZERO; 保留
	VECTOR 0x2B, ZERO; 保留
	VECTOR 0x2C, ZERO; ps/2 鼠标
	VECTOR 0x2D, ZERO; fpu 浮点单元异常
	VECTOR 0x2E, ZERO; 硬盘
	VECTOR 0x2F, ZERO; 保留

; 0x80中断
[bits 32]
extern syscall_table
section .text
global syscall_handler
syscall_handler:
	push 0; 中断若有错误码，会压在 eip 后面
	; 1 保存上下文环境
	push ds;
	push es;
	push fs;
	push gs;
	pushad;

	;push intr_str;
	;call put_str;
	;add esp, 4; 跳过参数

	; 如果是从片上进入的中断，除了往从片上发送EOI外，还要往主片上发送 EOI;
	mov al, EOI;  中断结束命令EOI，手动结束中断
	out 0xa0, al; 往从片发送, OCW2
	out 0x20, al; 往主片发送, 为了让8259A接受新的中断，必须要让8259A知道当前中断处理程序已经执行完成

	push 0x80;	// 为了统一栈格式
	; 2 为系统调用子功能传入参数
	push edx;	// 系统调用中第三个参数
	push ecx;	// 系统调用中第二个参数
	push ebx;	// 系统调用中第一个参数
	; 3 调用子功能处理函数
	call [syscall_table + eax * 4];	// 调C实现的中断处理函数
	add esp, 12; 跨过上面的3个参数
	; 4 将call调用后的返回值存入待当前内核栈中eax的位置
	mov [esp + (1+7)*4], eax; 1是push 0x80, 7是pushad压入7个寄存器的值，存入到eax的栈位置
	jmp intr_exit
	ret;
