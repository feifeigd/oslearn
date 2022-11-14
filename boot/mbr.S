
; 主引导程序
;------------------------------------------
section MBR vstart=0x7c00
	mov ax, cs
	
	mov ds, ax	
	mov es, ax
	mov fs, ax
	mov ss, ax
	
	mov sp, 0x7c00
	mov ax, 0xb800; 文本模式显示适配器地址
	mov gs, ax

	call clear
	call get_cursor
	; call print
	call set_screen

	jmp $	; 使程序悬停在此
	
; 清屏利用 0x06号功能,上卷全部行，则可清屏
;-------------------------------------------
; INT 0x10 功能号：0x06 功能描述：上卷窗口
;-------------------------------------------
; 输入：
; AH 功能号=0x06
; AL = 上卷的行数(如果是0, 表示全部)
; BH = 上卷行属性
; (CL, CH) = 窗口左上角的(X,Y)位置
; (DL, DH) = 窗口右下角的(X,Y)位置
; 无返回值
clear:
	mov ax, 0x600
	mov bx, 0x700
	mov cx, 0	; 左上角
	mov dx, 0x184f; 右下角(80, 25), 默认计算机显卡在加电后置为80*25文本模式
	int 0x10;
	ret
		
;;;;;;;;;;;;;;;;;;;;; 下面这三行代码获取光标位置;;;;;;;;;;;;;;;;;;;;
;.get_cursor 获取当前光标位置，在光标位置处打印字符
get_cursor:
	mov ah, 3; 输入：3号子功能是获取光标位置，需要存入ah
	mov bh, 0; bh 寄存器存储的是待获取光标的页号，第0页是默认页
	
	int 0x10; 输出；ch=光标开始行，cl=光标结束行，dh=光标所在行号，dl=光标所在列号
	ret
	
; 打印字符串，利用bios中断
print:
	mov ax, message
	mov bp, ax; es:bp 为串首地址，es此时同cs一致，开头时已经为sreg初始化
	
	; 光标位置要用到dx寄存器，cx中的光标位置可忽略
	mov cx, 5; cx为串长度，不包含结束符0
	mov ax, 0x1301; 子功能号13显示字符串及属性，要存入ah，al设置写字符串方式 ah=01:显示字符串，光标跟随移动
	mov bx, 0x14; bh 存储要显示的页号，此处是第0页，bl中是字符属性，属性黑底绿字(bl=02h)
	
	int 0x10;
	ret

; 操作显卡	
set_screen:
	mov ax, 0;
	mov cx, message_len
	mov byte gs:[0], '1'	
	mov byte[gs:0x01], 0xA4; A表示绿色背景闪烁，4表示前景色为红色

	mov byte[gs:0x02], ' '	
	mov byte[gs:0x03], 0xA4;

	mov byte[gs:0x04], 'M'	
	mov byte[gs:0x05], 0xA4;

	mov byte[gs:0x06], 'B'	
	mov byte[gs:0x07], 0xA4;

	mov byte[gs:0x08], 'R'	
	mov byte[gs:0x09], 0xA4;

	ret

	message db "1 MBR"
	message_len equ $ - message;

	times 510 - ($ - $$) db 0
	db 0x55, 0xaa
; nasm -f bin -o mbr.bin mbr.S
; dd if=mbr.bin of=../hd60M.img bs=512 count=1 conv=notrunc
