
[bits 32]

%define ERROR_CODE 	nop
%define ZERO 		push 0
extern put_str; 

section .data
intr_str db "interrupt occour!", 0xa, 0

global intr_entry_table

%macro VECTOR 2
section .text
intr%1entry:	; 每个中断处理程序都要压入中断向量号，所以一个中断类型一个中断处理程序
	%2
	push intr_str;
	call put_str;
%endmacro

intr_entry_table:
	VECTOR 0x00, ZERO
	VECTOR 0x01, ZERO
	VECTOR 0x02, ZERO
	VECTOR 0x03, ZERO
	VECTOR 0x04, ZERO
	VECTOR 0x05, ZERO
	VECTOR 0x06, ZERO
	VECTOR 0x07, ZERO
	VECTOR 0x08, ZERO
	VECTOR 0x09, ZERO
	VECTOR 0x0A, ZERO
	VECTOR 0x0B, ZERO
	VECTOR 0x0C, ZERO
	VECTOR 0x0D, ZERO
	VECTOR 0x0E, ZERO
	VECTOR 0x0F, ZERO
	
	VECTOR 0x10, ZERO
	VECTOR 0x11, ZERO
	VECTOR 0x12, ZERO
	VECTOR 0x13, ZERO
	VECTOR 0x14, ZERO
	VECTOR 0x15, ZERO
	VECTOR 0x16, ZERO
	VECTOR 0x17, ZERO
	VECTOR 0x18, ZERO
	VECTOR 0x19, ZERO
	VECTOR 0x1A, ZERO
	VECTOR 0x1B, ZERO
	VECTOR 0x1C, ZERO
	VECTOR 0x1D, ZERO
	VECTOR 0x1E, ERROR_CODE
	VECTOR 0x1F, ZERO
	VECTOR 0x20, ZERO
