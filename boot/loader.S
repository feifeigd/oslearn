%include "boot.inc"

section loader vstart=LOADER_BASE_ADDR
	mov ax, $$
	
	call set_screen

	label:
		;mov ax, $
	jmp label

; 操作显卡	
set_screen:
	;mov ax, 0;
	;mov cx, message_len
	mov byte gs:[0], 'L'	
	mov byte[gs:0x01], 0xA4; A表示绿色背景闪烁，4表示前景色为红色

	mov byte[gs:0x02], 'O'	
	mov byte[gs:0x03], 0xA4;

	mov byte[gs:0x04], 'A'	
	mov byte[gs:0x05], 0xA4;

	mov byte[gs:0x06], 'D'	
	mov byte[gs:0x07], 0xA4;

	mov byte[gs:0x08], 'E'	
	mov byte[gs:0x09], 0xA4;
	
	mov byte[gs:0x0A], 'R'	
	mov byte[gs:0x0B], 0xA4;

	ret
