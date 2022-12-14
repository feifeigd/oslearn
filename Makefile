BUILD_DIR = ./build

DISK_IMG = hd60M.img
DISK_IMG2 = hd50M.img
ENTRY_POINT = 0xc0001500

AS = nasm
ASFLAGS = -f elf
LIB = -I device/ -I kernel/ -I lib/ -I lib/kernel -I lib/user -I thread -I userprog

CFLAGS = -m32 -Wall $(LIB) -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector
LDFLAGS = -melf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map

HEADERS = device/console.h device/ioqueue.h device/keyboard.h device/timer.h		\
	kernel/debug.h kernel/global.h kernel/init.h kernel/interrupt.h	kernel/memory.h	\
	lib/kernel/bitmap.h												\
	lib/kernel/io.h													\
	lib/kernel/list.h												\
	lib/kernel/print.h												\
	lib/stdint.h													\
	lib/string.h													\
	thread/sync.h													\
	thread/thread.h													\
	userprog/tss.h

OBJS = $(BUILD_DIR)/main.o	\
	$(BUILD_DIR)/bitmap.o	\
	$(BUILD_DIR)/console.o	\
	$(BUILD_DIR)/debug.o	\
	$(BUILD_DIR)/init.o		\
	$(BUILD_DIR)/interrupt.o\
	$(BUILD_DIR)/ioqueue.o	\
	$(BUILD_DIR)/list.o		\
	$(BUILD_DIR)/kernel.o	\
	$(BUILD_DIR)/keyboard.o	\
	$(BUILD_DIR)/memory.o	\
	$(BUILD_DIR)/print.o	\
	$(BUILD_DIR)/process.o	\
	$(BUILD_DIR)/string.o	\
	$(BUILD_DIR)/switch.o	\
	$(BUILD_DIR)/sync.o		\
	$(BUILD_DIR)/syscall-init.o	\
	$(BUILD_DIR)/thread.o	\
	$(BUILD_DIR)/timer.o	\
	$(BUILD_DIR)/tss.o	

hd: mkdir mk_img $(BUILD_DIR)/kernel.bin
	@echo 写入内核
	dd if=$(BUILD_DIR)/kernel.bin of=$(DISK_IMG) bs=512 count=72 seek=9 conv=notrunc

# 汇编代码
$(BUILD_DIR)/print.o: lib/kernel/print.S
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/kernel.o: kernel/kernel.S
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/switch.o: thread/switch.S
	$(AS) $(ASFLAGS) $< -o $@


# C 代码编译
$(BUILD_DIR)/main.o: kernel/main.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/bitmap.o: lib/kernel/bitmap.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/console.o: device/console.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/debug.o: kernel/debug.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/init.o: kernel/init.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: kernel/interrupt.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/ioqueue.o: device/ioqueue.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/keyboard.o: device/keyboard.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/list.o: lib/kernel/list.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/memory.o: kernel/memory.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/process.o: userprog/process.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/sync.o: thread/sync.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/string.o: lib/string.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/syscall-init.o: userprog/syscall-init.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/timer.o: device/timer.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/thread.o: thread/thread.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/tss.o: userprog/tss.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

########## 链接所有目标
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

# 伪目标
.PHONY : mk_dir clean all

mk_img:
	@if [ ! -e $(DISK_IMG) ]; then bximage -q -hd=10M -func=create $(DISK_IMG); fi

mkdir:
	@if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi


ctags:
	ctags -R

clean:
	@echo 清理
	cd $(BUILD_DIR) && rm -f ./* 

build: $(BUILD_DIR)/kernel.bin 

all: clean mk_dir build hd ctags



