%include "boot.inc"

LOADER_STACK_TOP equ LOADER_BASE_ADDR

[bits 16]
; loader 的大小，不能超过64KB，因为实模式下偏移地址是16位，只能访问 0~FFFFh
section loader vstart=LOADER_BASE_ADDR ; b 0x900
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
	;jmp $;
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
	mov cx, KB1; 1KB,cx 和 ax值一样，cx用作乘数
	mul cx;
	shr edx, 16
	and eax, 0x0000FFFF;
	or edx, eax;
	add edx, MB1; ax 只是15MB，故要加上1MB
	mov esi, edx; 先把15MB的内存量存入esi寄存器备份
; 2 再将16MB以上的内存转换为byte为单位
; 寄存器 bx 和 dx中是以64KB为单位的内存数量
	xor eax, eax;
	mov ax, bx;
	mov ecx, KB64; 64KB
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
	mov cx, KB1; 将 ax 转为byte为单位
	mul cx;
	shl edx, 16; 把dx移到高16位
	or edx, eax;
	add edx, MB1; 0x88 子功能只会返回1MB 以上的内存，故实际内存大小要加上1MB
.error_hlt:
	hlt
	;jmp $
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
	
;---------------- 加载 kernel
	mov eax, KERNEL_START_SECTOR; kernel.bin 所在的扇区号
	mov ebx, KERNEL_BIN_BASE_ADDR;
	; 从磁盘读出后，写入到 ebx指定的地址
	mov ecx, KERNEL_SECTOR_COUNT; 读入的扇区数
	call rd_disk_m_32;

	; 创建页目录及页表并初始化内存位图
	call setup_page; b 0x2cd

	; 将描述符表地址及偏移量写入内存 gdt_ptr, 一会儿用新地址重新加载
	sgdt [gdt_ptr]	; 存到原来 gdt所在的位置

	; 将gdt描述符中视频段描述符中的基地址+0xc0000000
	mov ebx, [gdt_ptr + 2]
	or dword[ebx + (VIDEO_DESC - GDT_BASE) + 4], 0xc0000000; 视频段是第3个段描述符，每个描述符8字节，故 8*3+4=0x18。段描述符的高4字节的最高位的段基址的 31~24位

	; 将gdt的基址加上0xc0000000，使其成为内核所在的高地址
	or dword[gdt_ptr + 2], 0xc0000000

	or esp, 0xc0000000; 将栈指针同样映射为内核地址

	; 把页目录地址赋值给 cr3
	mov eax, PAGE_DIR_TABLE_POS
	mov cr3, eax;

	; 打开cr0的page位(第31位)
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax;

	; 开启分页后，用gdt新的地址重新加载
	lgdt [gdt_ptr]; 重新加载

	call set_screen
	jmp SELECTOR_CODE: enter_kernel; b 0xc7f
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

show_kernel:
	mov byte gs:[0x140], 'K';
	mov byte gs:[0x142], 'E';
	mov byte gs:[0x144], 'R';
	mov byte gs:[0x146], 'N';
	mov byte gs:[0x148], 'E';
	mov byte gs:[0x14A], 'L';

	ret;

;-------------------- 创建页目录及页表
; 需要物理内存 = 1MB + 2PAGE + 254PAGE=1MB + 256PAGE = 2MB
setup_page:	; b 0xca8
; 先把页目录占用空间清零
	mov ecx, KB4; 4KB
	mov esi, 0
.clear_page_dir:
	mov byte[PAGE_DIR_TABLE_POS + esi], 0
	inc esi
	loop .clear_page_dir

; 开始创建页目录项(PDE)
.create_pde: ; 创建 Page Directory Entry
	mov eax, PAGE_DIR_TABLE_POS
	add eax, PAGE1 * 1; 此时eax 为第一个页表的位置及属性
	mov ebx, eax; 为 .create_pte 做准备

; 下面将页表项 0 和 0xc00 都存为第一个页表的地址，每个页表 表示4KB/4B*4KB=4MB内存
; 这样0xc03fffff 以下的地址和 0x003fffff 以下的地址都指向相同的页表
; 这是为将地址映射为内核地址做准备
	or eax, PAGE_US2_U | PAGE_RW1_W | PAGE_P0; 页表项的属性RW和P为1，US为1，表示用户属性，所有特权级别都可以访问
	mov [PAGE_DIR_TABLE_POS + 0x0], eax; 第一个目录项，在页目录表中的第一个目录项写入第一个页表的位置(0x101000)及属性(7)
	mov [PAGE_DIR_TABLE_POS + 0xc00], eax; 一个页表项占用4字节，0xc00 表示第0xc00/4=768个页表占用的目录项，0xc00以上的目录项用于内核空间，
	; 也就是页表的 0xc0000000 ~ 0xffffffff 共计1G属于内核
	; 0x0 ~ 0xbfffffff 共计3G属于用户进程
	; sub eax, PAGE1
	mov eax, PAGE_DIR_TABLE_POS
	mov [PAGE_DIR_TABLE_POS + PAGE1 - 4], eax; 使最后一个目录项指向页目录表自己的地址

; 下面创建页表项(PTE)
	mov ecx, MB1 / KB4; 有多少个页表项, 1MB低端内存/每页大小4KB， MB1 / KB4 = 256.  还有3MB没初始化
	mov esi, 0
	mov edx, 0 | PAGE_US2_U | PAGE_RW1_W | PAGE_P0; US=1,RW=1,P=1. 1MB 低端内存
.create_pte: ; Page Table Entry
	mov [ebx + esi * 4], edx; 	// ebx 为第0个页表，第0个页表指向  0~1MB-1B 的PAGE_DIR_TABLE_POS前的低1M内存内存
	inc esi;
	add edx, PAGE1;	下一页
	loop .create_pte

; 创建内核其他的PDE 
	mov eax, PAGE_DIR_TABLE_POS	
	add eax, PAGE1 * 2; 此时eax 为第二个页表的位置
	or  eax, PAGE_US2_U | PAGE_RW1_W | PAGE_P0
	mov ebx, PAGE_DIR_TABLE_POS
	mov esi, (0xc00 + 4) / 4; 769 ~ 1022 个页目录项， (0xc00 + 4) / 4 = 769，第769个页目录项的偏移
	mov ecx, 1022 - (0xc00 + 4) / 4 + 1 ; 1022 - esi + 1 = 254 个

.create_kernel_pde: ; Page Directory Entry
	mov [ebx + esi * 4], eax; 	// ebx 页目录表地址
	inc esi;
	add eax, PAGE1;	下一页
	loop .create_kernel_pde

	ret

;---------------------------------------
; 功能：读取硬盘n个扇区
; 抄一份 rd_disk_m_16，并将 bx改为 ebx
rd_disk_m_32:
;---------------------------------------
	; eax=LBA扇区号
	; ebx=将数据写入的内存地址
	; ecx=读入的扇区数
	mov esi, eax; 备份
	mov di, cx; 备份扇区数
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
	mov dx, 256; 512 / 2
	mul dx
	mov cx, ax; di 为要读取的扇区数， 一个扇区有512字节，每次读入一个字，共需 di * 512 /2 = di * 256
	mov dx, 0x1f0
.go_on_read:
	in ax, dx
	mov [ebx], ax
	add ebx, 2
	loop .go_on_read
	
	ret;

enter_kernel:
	call show_kernel
	call kernel_init; b 0xe0a
	mov esp, 0xc009f000; 内核主线程的栈
	jmp KERNEL_ENTRY_POINT; b 0xe14
	jmp $;

;------------------------- kernel.bin 中的segment拷贝到编译的地址
; typedef struct elf32_hdr{
;  unsigned char e_ident[EI_NIDENT];0
;  Elf32_Half    e_type;0x10
;  Elf32_Half    e_machine;0x12
;  Elf32_Word    e_version;0x14
;  Elf32_Addr    e_entry;0x18  /* Entry point */
;  Elf32_Off     e_phoff;0x1c
;  Elf32_Off     e_shoff;0x20
;  Elf32_Word    e_flags;0x24
;  Elf32_Half    e_ehsize;0x28
;  Elf32_Half    e_phentsize;0x2a
;  Elf32_Half    e_phnum;0x2c
;  Elf32_Half    e_shentsize;0x2e
;  Elf32_Half    e_shnum;0x30
;  Elf32_Half    e_shstrndx;0x32
;} Elf32_Ehdr;

kernel_init:
	xor eax, eax;
	xor ebx, ebx; 记录程序头表地址
	xor ecx, ecx; 记录程序头表中的program header数量
	xor edx, edx; 记录program header 尺寸，即 e_phentsize

	mov ebx, [KERNEL_BIN_BASE_ADDR + 0x1c]; e_phoff
	add ebx, KERNEL_BIN_BASE_ADDR;
	mov cx, [KERNEL_BIN_BASE_ADDR + 0x2c]; e_phnum
	mov dx, [KERNEL_BIN_BASE_ADDR + 0x2a]; e_phentsize

; typedef struct elf32_phdr{
;  Elf32_Word    p_type; 0x0
;  Elf32_Off     p_offset;0x4
;  Elf32_Addr    p_vaddr;0x8
;  Elf32_Addr    p_paddr;0xc
;  Elf32_Word    p_filesz;0x10
;  Elf32_Word    p_memsz;
;  Elf32_Word    p_flags;
;  Elf32_Word    p_align;
;} Elf32_Phdr;
.each_segment:
	cmp byte[ebx + 0], PT_NULL; 若  p_type = PT_NULL,说明此program header 未使用
	je .PTNULL

	; memcpy(dst, src, size)
	push dword [ebx + 0x10]; p_filesz
	mov eax, [ebx + 0x4];p_offset
	add eax, KERNEL_BIN_BASE_ADDR;
	push eax
	push dword[ebx + 0x8]; p_vaddr, 目的虚拟地址
	call mem_cpy; b 0xe51
	add esp, 0xc; 清理栈中压入的三个参数
.PTNULL
	add ebx, edx; 指向下一个 program header 
	loop .each_segment
	ret;

; 内存拷贝
mem_cpy:
	cld;
	push ebp
	mov ebp, esp;
	push ecx;

	mov edi, [ebp + 8]; dst = 08048000
	mov esi, [ebp + 0xc]; src
	mov ecx, [ebp + 0x10]; size
	rep movsb; 逐字节拷贝

	pop ecx;
	pop ebp;
	ret;

[bits 64]
