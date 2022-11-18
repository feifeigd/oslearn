%include "boot.inc"

LOADER_STACK_TOP equ LOADER_BASE_ADDR

[bits 16]
; loader 的大小，不能超过64KB，因为实模式下偏移地址是16位，只能访问 0~FFFFh
section loader vstart=LOADER_BASE_ADDR
	jmp loader_start;
	

; 构建 gdt 及其内部的描述符
; 一个段描述符为8个字节，第0个全局描述符不使用，故全0
GDT_BASE: ; Null Descriptor
	dd 0x00000000
	dd 0x00000000

; 代码段描述符
CODE_DESC: ; Kernel Mode Code Segment
	dd 0x0000FFFF	; 低16位表示段界限的 bit15~0 0xFFFF
	dd DESC_CODE_HIGH4; DESC_CODE_HIGH4 中 DESC_LIMIT19_16_CODE2 4位1，表示 0xF, 整个段界限合并为 0xFFFFF
DATA_STACK_DESC:
	dd 0x0000FFFF
	dd DESC_DATA_HIGH4
VIDEO_DESC:
	dd 0x80000007; 文本模式 limit=(0xbffff-0xb8000) / 4k = 0x0007, 段基址=(DESC_VIDEO_HIGH4中0x00与0xb 与 0x8000 合成 0x000b8000)
	dd DESC_VIDEO_HIGH4

GDT_SIZE	equ $ - GDT_BASE
GDT_LIMIT	equ GDT_SIZE - 1
; times 60 dq 0; 此处预留60个描述符的空位

; 物理内存的大小
; 想办法把这个偏移弄成 0x200
times (0x200 - ($ - $$)) db 0
total_mem_bytes dd 0; xp /w 0xb00; 0x900+0x200=0xb00

; 选择子
SELECTOR_CODE	equ (CODE_DESC			- GDT_BASE) | TI2_GDT | RPL1_0_0; 相当于 ((CODE_DESC - GDT_BASE) / 8) << 3 + TI2_GDT + RPL1_0_0
SELECTOR_DATA	equ (DATA_STACK_DESC	- GDT_BASE) | TI2_GDT | RPL1_0_0;
SELECTOR_VIDEO	equ (VIDEO_DESC			- GDT_BASE) | TI2_GDT | RPL1_0_0;


; gdt 的指针, 前2字节是gdt 界限，后4字节是gdt起始地址
gdt_ptr:
	dw GDT_LIMIT
	dd GDT_BASE

ards_buf:
BaseAddrLow 	dd 0
BaseAddrHigh 	dd 0
LengthLow 		dd 0
LengthHigh		dd 0
Type			dd 0

ards_nr dw 0; 用于记录ARDS的结构体数量

loadermsg 		db '2 loader in real.'
loadermsg_len	equ $ - loadermsg

loader_start:
	call print
	call detect_memory

; ----------------------- 进入保护模式; 
; 1 打开A20
	in al, 0x92
	or al, 0000_0010b
	out 0x92, al
; 2 加载gdt
	lgdt [gdt_ptr]

; 3 打开 PE 位
	mov eax, cr0
	or eax, 0x00000001
	mov cr0, eax;

	jmp dword SELECTOR_CODE: p_mode_start;	刷新流水线
	jmp $;
	; 不会回来这里了

; 打印字符串，利用bios中断
print:
	;mov sp, LOADER_BASE_ADDR
	mov bp, loadermsg; es:bp 为串首地址，es此时同cs一致，开头时已经为sreg初始化
	
	; 光标位置要用到dx寄存器，cx中的光标位置可忽略
	mov cx, loadermsg_len; cx为串长度，不包含结束符0
	mov ax, 0x1301; 子功能号13显示字符串及属性，要存入ah，al设置写字符串方式 ah=01:显示字符串，光标跟随移动
	mov bx, 0x001f; bh 存储要显示的页号，此处是第0页，bl中是字符属性，属性黑底绿字(bl=02h)
	mov dx, 0x1800;	// 最后那行 0x18行，0列
	int 0x10;
	ret

; 获取物理内存大小, 从强到弱 0xe820 -> 0xe801 -> 0x88
detect_memory:
; int 15h eax=0000E820h, edx=534D4150h('SMAP')获取内存布局
	xor ebx, ebx; 第一次调用时 ebx值要是0.只赋值一次，循环体中不会改变
	mov edx, 534D4150h
	mov di, ards_buf; es 已经在mbr中赋值
	xor esi, esi; 存放最大内存大小
.e820_mem_get_loop:
	mov eax, 0x0000e820; 执行int 15h后，eax值变为0x534d4150,eax/ebx/ecx都会更新，所以每次循环要更新为子功能号
	mov ecx, 14h; ARDS 地址范围描述符结构体大小是20字节

	int 15h

	jc .e820_failed_so_try_e801; 若 cf位为1则有错误发生，尝试 0xe801子功能

	inc word [ards_nr]; 

	cmp ebx, 0; 若 ebx为0且cf不为1，这说明ards全部返回
	jz .end_e820_mem_get_loop

; 在所有ards结构体中，找出 (base_add_low + length_low) 的最大值，即内存的容量

; 无需判断type是否为1，最大的内存块一定是可被使用的
	mov eax, [BaseAddrLow]; base_addr_low
	add eax, [LengthLow]; length_low

	cmp esi, eax; 冒泡排序，找出最大，edx寄存器始终是最大的内存容量
	jge .next_ards
	mov esi, eax; 替换最大值
.next_ards:
	jmp .e820_mem_get_loop
.end_e820_mem_get_loop:

	mov edx, esi
	jmp .mem_get_ok

;--------- int 15h, ax=e801h获取内存大小，最大支持4G
; 返回后， ax cx值一样，以KB为单位, bx dx值一样，以64KB 为单位
; 在 ax和cx寄存器中为低16MB，在bx和dx中为16MB到4G
.e820_failed_so_try_e801:
	mov ax, 0xe801;
	int 15h;
	jc .e801_failed_so_try88; 若当前e801方法失败，就尝试0x88方法
; 1 先算出低15MB的内存
; ax 和 cx 中是以KB为单位的内存数量，将其转换为以byte为单位
	mov cx, 0x400; 1KB,cx 和 ax值一样，cx用作乘数
	mul cx;
	shr edx, 16
	and eax, 0x0000FFFF;
	or edx, eax;
	add edx, 0x100000; ax 只是15MB，故要加上1MB
	mov esi, edx; 先把15MB的内存量存入esi寄存器备份
; 2 再将16MB以上的内存转换为byte为单位
; 寄存器 bx 和 dx中是以64KB为单位的内存数量
	xor eax, eax;
	mov ax, bx;
	mov ecx, 0x10000; 64KB
	mul ecx 
	add esi, eax; 由于此方法只能测出4GB以内的内存，故32位eax足够了, edx肯定为0，只加eax即可
	mov edx, esi; edx 为总内存大小
	jmp .mem_get_ok

; ------ int 15h,ah=0x88获取内存大小，只能获取64MB之内
.e801_failed_so_try88:
	; int 15h后，ax存入的是以KB为单位的内存容量
	mov ah, 0x88;
	int 0x15
	jc .error_hlt
	and eax, 0x0000FFFF
	; 16位乘法积为32位，积的高16位在dx中，积的低16位在ax中
	mov cx, 0x400; 将 ax 转为byte为单位
	mul cx;
	shl edx, 16; 把dx移到高16位
	or edx, eax;
	add edx, 0x100000; 0x88 子功能只会返回1MB 以上的内存，故实际内存大小要加上1MB
.error_hlt:
	jmp $
.mem_get_ok:
	mov [total_mem_bytes], edx; 获得了内存容量 b 0xc03

	ret;

[bits 32]
p_mode_start:
	mov ax, SELECTOR_DATA
	mov ds, ax
	mov es, ax
	mov ss, ax

	mov esp, LOADER_STACK_TOP

	mov ax, SELECTOR_VIDEO
	MOV gs, ax

	mov byte gs:[0xA0], 'P'	
	call set_screen

	jmp $

; 操作显卡	
set_screen:
	mov byte gs:[0xA0], '2'; 覆盖 P
	;mov byte[gs:0xA1], 0xA4; A表示绿色背景闪烁，4表示前景色为红色

	mov byte[gs:0xA2], ' '	
	mov byte[gs:0xA3], 0xA4;

	mov byte[gs:0xA4], 'L'	
	mov byte[gs:0xA5], 0xA4;

	mov byte[gs:0xA6], 'O'	
	mov byte[gs:0xA7], 0xA4;

	mov byte[gs:0xA8], 'A'	
	mov byte[gs:0xA9], 0xA4;

	mov byte[gs:0xAA], 'D'	
	mov byte[gs:0xAB], 0xA4;

	mov byte[gs:0xAC], 'E'	
	mov byte[gs:0xAD], 0xA4;

	mov byte[gs:0xAE], 'R'	
	mov byte[gs:0xAF], 0xA4;

	ret

[bits 64]
