BUILD_DIR = ./build

DISK_IMG = hd60m.img
DISK_IMG2 = hd50m.img
ENTRY_POINT = 0xc0001500

CFLAGS = -m32 -Wall $(LIB) -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector
LDFLAGS = -m32 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map

OBJS = $(BUILD_DIR)/main.o

hd: mkdir $(BUILD_DIR)/kernel.bin
	echo 写入内核
	dd if=$(BUILD_DIR)/kernel.bin of=hd60M.img bs=512 count=20 seek=9 conv=notrunc

# C 代码编译
$(BUILD_DIR)/main.o: kernel/main.c
	$(CC) $(CFLAGS) $< -o $@

########## 链接所有目标
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY : mk_dir clean all


mkdir:
	if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi

ctags:
	ctags -R

clean:
	echo 清理
	cd $(BUILD_DIR) && rm -f ./* 

build: $(BUILD_DIR)/kernel.bin 

all: clean mk_dir build hd ctags



