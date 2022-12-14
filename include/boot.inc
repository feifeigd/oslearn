
;--------------------- loader 和 kernel
LOADER_BASE_ADDR 	equ 0x900
loader_start_offset equ 0x300
LOADER_START_SECTOR equ 0x2; LBA 地址，第2块扇区. 第0个扇区是MBR，第1个扇区空置还没使用
LOADER_SECTOR_COUNT equ 4; loader 的大小占用多少个扇区

KERNEL_BIN_BASE_ADDR	equ 0x70000
KERNEL_START_SECTOR		equ 0x9
KERNEL_ENTRY_POINT		equ 0xc0001500; kernel入口虚拟地址
KERNEL_SECTOR_COUNT		equ 72; kernel.bin 的大小占用多少个扇区

;-------------- 页表配置
PAGE_DIR_TABLE_POS	equ 0x00100000; 页目录表物理地址， 起始1MB

KB1		equ 0x400; 1KB
KB4 	equ 0x1000; 4KB
KB64	equ 0x10000; 64KB
MB1 	equ 0x100000; 1MB
PAGE1	equ KB4
;---------------------------- gdt 描述符属性

; 段描述符高4字节的第23bit是G位(Granularity 粒度)，G=0表示段界限单位是 1B，G=1表示段界限单位是4KB；段界限用20个bit表示
DESC_G23_1B     		equ 00000000_00000000_00000000_00000000b; G=0
DESC_G23_4KB    		equ 00000000_10000000_00000000_00000000b; G=1

; 段描述符高4字节的第22bit是D/B位

; 对代码段来说，此位是D位
; D=0 表示指令中的有效地址和操作数是16bit，指令有效地址用IP寄存器；
; D=1 表示指令中的有效地址及操作数是32bit，指令有效地址用EIP寄存器
DESC_D22_32     		equ 00000000_01000000_00000000_00000000b; D=1

; 对栈段来说，此位是B位
; B=0, 使用sp，16bit寄存器最大寻址范围 0xFFFF
; B=1, 使用esp，32bit寄存器最大寻址范围 0xFFFFFFFF

; 段描述符高4字节的第21bit为L字段，用来设置是否64位代码段，L=1表示64位代码段，L=0表示32位代码段
DESC_L21        		equ 00000000_00000000_00000000_00000000b; 64位代码段，此处标记为0

; 段描述符高4字节的第20bit
DESC_AVL20      		equ 00000000_00000000_00000000_00000000b; CPU 不使用此位，暂置为0

; 段界限的第2部分，bit19~16,4个bit，跟段描述符低4字节中的低2字节，共组成 20bit的段界限
DESC_LIMIT19_16_CODE2   equ 00000000_00001111_00000000_00000000b;
DESC_LIMIT19_16_DATA2   equ DESC_LIMIT19_16_CODE2;
DESC_LIMIT19_16_VIDEO2	equ 00000000_00000000_00000000_00000000b;

DESC_P15        		equ 00000000_00000000_10000000_00000000b; Present 是否在内存中
; DESC_DPL14_13   equ 00000000_00000000_00000000_00000000b; 描述符特权级 0~3
DESC_DPL14_13_0      	equ 00000000_00000000_00000000_00000000b; 描述符特权级 0
DESC_DPL14_13_1      	equ 00000000_00000000_00100000_00000000b; 描述符特权级 1
DESC_DPL14_13_2      	equ 00000000_00000000_01000000_00000000b; 描述符特权级 2
DESC_DPL14_13_3      	equ 00000000_00000000_01100000_00000000b; 描述符特权级 3

; DESC_S12        equ 00000000_00000000_00000000_00000000b; 是否系统段，S=0系统段，S=1非系统段
DESC_S12_SYS			equ 00000000_00000000_00000000_00000000b; S=0系统段
DESC_S12_CODE        	equ 00000000_00000000_00010000_00000000b; S=1非系统段
DESC_S12_DATA        	equ DESC_S12_CODE;

; DESC_TYPE11_8   equ 00000000_00000000_00000000_00000000b; 共4bit，分系统段 与 非系统段
DESC_TYPE11_8_CODE 		equ 00000000_00000000_00001000_00000000b; X=1,C=0,R=0,A=0; 代码段是可执行的，非一致性，不可读，已访问位A清零
DESC_TYPE11_8_DATA 		equ 00000000_00000000_00000010_00000000b; X=0,E=0,W=1,A=0; 数据段不可执行的，向上扩展的，可读写，已访问位A清零


; 代码段的高4字节
DESC_CODE_HIGH4 equ			\
	  (0x00 << 24) 			\
	| DESC_G23_4KB			\
	| DESC_D22_32			\
	| DESC_L21				\
	| DESC_AVL20			\
	| DESC_LIMIT19_16_CODE2	\
	| DESC_P15				\
	| DESC_DPL14_13_0		\
	| DESC_S12_CODE			\
	| DESC_TYPE11_8_CODE	\
	| 0x00


; 数据段的高4字节
DESC_DATA_HIGH4 equ			\
	  (0x00 << 24) 			\
	| DESC_G23_4KB			\
	| DESC_D22_32			\
	| DESC_L21				\
	| DESC_AVL20			\
	| DESC_LIMIT19_16_DATA2	\
	| DESC_P15				\
	| DESC_DPL14_13_0		\
	| DESC_S12_DATA			\
	| DESC_TYPE11_8_DATA	\
	| 0x00

; 显存的高4字节
DESC_VIDEO_HIGH4 equ		\
	  (0x00 << 24) 			\
	| DESC_G23_4KB			\
	| DESC_D22_32			\
	| DESC_L21				\
	| DESC_AVL20			\
	| DESC_LIMIT19_16_VIDEO2\
	| DESC_P15				\
	| DESC_DPL14_13_0		\
	| DESC_S12_DATA			\
	| DESC_TYPE11_8_DATA	\
	| 0x0b	; 原文这里是0，原文是错误的。段基址 bit23~16, 

;----------------- 选择子属性
RPL1_0_0 equ 00b;
RPL1_0_1 equ 01b;
RPL1_0_2 equ 10b;
RPL1_0_3 equ 11b;

TI2_GDT equ 000b; 在GDT中
TI2_LDT equ 100b; 在LDT中

;---------------------------- 页表相关属性
PAGE_P0		equ   1b; Present
PG_P equ PAGE_P0
PAGE_RW1_R	equ  00b; 可读不可写
PAGE_RW1_W	equ  10b; 可读可写
PG_RW_W equ PAGE_RW1_W
PAGE_US2_S	equ 000b; Supervisor
PAGE_US2_U	equ 100b; User
PG_US_U equ PAGE_US2_U

;------------------- program type 定义
PT_NULL equ 0
