
%include "boot.inc"

bits 16
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

	mov eax, LOADER_START_SECTOR; 起始扇区lab地址
	mov bx, LOADER_BASE_ADDR; 写入的地址
	mov cx, LOADER_SECTOR_COUNT; 待读入的扇区数
	call rd_disk_m_16; 以下读取程序的其实部分(一个扇区)
	jmp LOADER_BASE_ADDR;
	
	;jmp $	; 使程序悬停在此
	
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
;print:
;	mov ax, message
;	mov bp, ax; es:bp 为串首地址，es此时同cs一致，开头时已经为sreg初始化
	
	; 光标位置要用到dx寄存器，cx中的光标位置可忽略
;	mov cx, 5; cx为串长度，不包含结束符0
;	mov ax, 0x1301; 子功能号13显示字符串及属性，要存入ah，al设置写字符串方式 ah=01:显示字符串，光标跟随移动
;	mov bx, 0x14; bh 存储要显示的页号，此处是第0页，bl中是字符属性，属性黑底绿字(bl=02h)
	
;	int 0x10;
;	ret

; 操作显卡	
set_screen:
	;mov ax, 0;
	;mov cx, message_len
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

; 读取硬盘N个扇区
rd_disk_m_16:
	; eax=LBA扇区号
	; bx=将数据写入的内存地址
	; cx=读入的扇区数
	mov esi, eax; 备份
	mov di,cx; 备份
; 读写硬盘：
; 第一步：设置要读取的扇区数
	mov dx, 0x1f2; sector count
	mov al, cl
	out dx, al; 读取是扇区数
	mov eax, esi; 恢复ax
; 第二步：将LBA地址存入 0x1f3 ~ 0x1f6
	; LBA地址7~0位写入端口 0x1f3
	mov dx, 0x1f3
	out dx, al
	; LBA地址15~8位写入端口0x1f4
	mov cl, 8
	shr eax, cl
	mov dx, 0x1f4
	out dx, al
	; LBA 地址23~16位写入端口0x1f5
	shr eax, cl
	mov dx, 0x1f5
	out dx, al
	
	shr eax, cl
	and al, 0xf; LBA 第24~27位
	or al,0xe0; 设置7~4位为 1110， 表示lba模式
	mov dx, 0x1f6
	out dx, al

; 第三步： 向 0x1f7端口写入读命令， 0x20
	mov dx, 0x1f7;
	mov al, 0x20; 
	out dx, al
; 第四步： 检测硬盘状态
.not_ready:
	; 同一个端口，写时表示写入命令字，读时表示读入硬盘状态
	nop
	in al, dx
	and al, 0x88; 第四位为1表示硬盘控制器已准备好数据传输; 第七位为1表示硬盘忙
	cmp al, 0x08
	jnz .not_ready;	继续等
; 第五步：从0x1f0端口读数据
	mov ax, di
	mov dx, 256
	mul dx
	mov cx, ax; di 为要读取的扇区数， 一个扇区有512字节，每次读入一个字，共需 di * 512 /2 = di * 256
	mov dx, 0x1f0
.go_on_read:
	in ax, dx
	mov [bx], ax
	add bx, 2
	loop .go_on_read
	
	ret;
	
	message db "1 MBR"
	message_len equ $ - message;

	times 510 - ($ - $$) db 0
	db 0x55, 0xaa
; nasm -f bin -o mbr.bin mbr.S
; dd if=mbr.bin of=../hd60M.img bs=512 count=1 conv=notrunc
