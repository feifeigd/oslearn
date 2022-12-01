BUILD_DIR = ./build

DISK_IMG = hd60M.img
DISK_IMG2 = hd50M.img
ENTRY_POINT = 0xc0001500

AS = nasm
ASFLAGS = -f elf
LIB = -I kernel/ -I lib/kernel

CFLAGS = -m32 -Wall $(LIB) -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector
LDFLAGS = -melf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map

OBJS = $(BUILD_DIR)/main.o	\
	$(BUILD_DIR)/init.o		\
	$(BUILD_DIR)/interrupt.o		\
	$(BUILD_DIR)/kernel.o	\
	$(BUILD_DIR)/print.o

hd: mkdir mk_img $(BUILD_DIR)/kernel.bin
	echo 写入内核
	dd if=$(BUILD_DIR)/kernel.bin of=$(DISK_IMG) bs=512 count=22 seek=9 conv=notrunc

# 汇编代码
$(BUILD_DIR)/print.o: lib/kernel/print.S
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/kernel.o: kernel/kernel.S
	$(AS) $(ASFLAGS) $< -o $@


# C 代码编译
$(BUILD_DIR)/main.o: kernel/main.c
	$(CC) $(CFLAGS) $< -o $@

kernel/init.h:
	echo kernel/init.h 更新了
kernel/interrupt.h: lib/stdint.h
	echo kernel/interrupt.h 更新了

kernel/print.h:
	echo kernel/print.h 更新了
kernel/init.h: kernel/print.h
	echo kernel/init.h: kernel/print.h

lib/kernel/print.h:
	echo lib/kernel/print.h 更新了

lib/stdint.h:
	echo lib/stdint.h 更新了

$(BUILD_DIR)/init.o: kernel/init.c kernel/init.h kernel/print.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: kernel/interrupt.c kernel/interrupt.h
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



