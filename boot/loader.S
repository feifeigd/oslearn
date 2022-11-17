%include "boot.inc"

LOADER_STACK_TOP equ LOADER_BASE_ADDR

[bits 16]
; loader 的大小，不能超过64KB，因为实模式下偏移地址是16位，只能访问 0~FFFFh
section loader vstart=LOADER_BASE_ADDR
	mov ax, $$
	
	call set_screen

	label:
		;mov ax, $
	jmp label

; 操作显卡	
set_screen:
	mov byte gs:[0], '2'	
	mov byte[gs:0x01], 0xA4; A表示绿色背景闪烁，4表示前景色为红色

	mov byte[gs:0x02], ' '	
	mov byte[gs:0x03], 0xA4;

	mov byte[gs:0x04], 'L'	
	mov byte[gs:0x05], 0xA4;

	mov byte[gs:0x06], 'O'	
	mov byte[gs:0x07], 0xA4;

	mov byte[gs:0x08], 'A'	
	mov byte[gs:0x09], 0xA4;

	mov byte[gs:0x0A], 'D'	
	mov byte[gs:0x0B], 0xA4;

	mov byte[gs:0x0C], 'E'	
	mov byte[gs:0x0D], 0xA4;

	mov byte[gs:0x0E], 'R'	
	mov byte[gs:0x0F], 0xA4;

	ret

; 构建 gdt 及其内部的描述符
; 一个段描述符为8个字节，第0个全局描述符不使用，故全0
GDT_BASE: 
	dd 0x00000000
	dd 0x00000000
CODE_DESC:
	dd 0x0000FFFF
	dd DESC_CODE_HIGH4

[bits 32]
