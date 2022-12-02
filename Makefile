BUILD_DIR = ./build

DISK_IMG = hd60M.img
DISK_IMG2 = hd50M.img
ENTRY_POINT = 0xc0001500

AS = nasm
ASFLAGS = -f elf
LIB = -I kernel/ -I lib/ -I lib/kernel

CFLAGS = -m32 -Wall $(LIB) -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector
LDFLAGS = -melf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map

HEADERS = device/timer.h 								\
	kernel/global.h kernel/init.h kernel/interrupt.h 	\
	lib/kernel/io.h										\
	lib/kernel/print.h									\
	lib/stdint.h

OBJS = $(BUILD_DIR)/main.o	\
	$(BUILD_DIR)/init.o		\
	$(BUILD_DIR)/interrupt.o\
	$(BUILD_DIR)/kernel.o	\
	$(BUILD_DIR)/print.o	\
	$(BUILD_DIR)/timer.o	

hd: mkdir mk_img $(BUILD_DIR)/kernel.bin
	echo 写入内核
	dd if=$(BUILD_DIR)/kernel.bin of=$(DISK_IMG) bs=512 count=32 seek=9 conv=notrunc

# 汇编代码
$(BUILD_DIR)/print.o: lib/kernel/print.S
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/kernel.o: kernel/kernel.S
	$(AS) $(ASFLAGS) $< -o $@


# C 代码编译
$(BUILD_DIR)/main.o: kernel/main.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o: kernel/init.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: kernel/interrupt.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/timer.o: device/timer.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

########## 链接所有目标
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

# 伪目标
.PHONY : mk_dir clean all

mk_img:
	if [ ! -e $(DISK_IMG) ]; then bximage -q -hd=10M -func=create $(DISK_IMG); fi

mkdir:
	if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi


ctags:
	ctags -R

clean:
	echo 清理
	cd $(BUILD_DIR) && rm -f ./* 

build: $(BUILD_DIR)/kernel.bin 

all: clean mk_dir build hd ctags



