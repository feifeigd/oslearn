
;--------------------- loader 和 kernel
LOADER_BASE_ADDR 	equ 0x900
LOADER_START_SECTOR equ 0x2; LBA 地址，第2块扇区. 第0个扇区是MBR，第1个扇区空置还没使用
LOADER_SECTOR_COUNT equ 1; loader的大小占用多少个扇区
