#include "debug.h"
#include "memory.h"
#include <print.h>
#include <stdint.h>
#include <string.h>

#define PG_SIZE 4096

// 位图地址
// 因为 0xc009f000 是内核主线程的栈顶，因为 0xc009e000 是内核主线程的pcb
// 一个页框大小的位图可表示128MB内存，位图位置安排在地址 0xc009a000
// 这样本系统最大支持4个页框的位图，即512MB
#define MEM_BITMAP_BASE 0xc009a000

// 0xc0000000 是内核从虚拟地址3G起
// 0x100000 意指跨过低端 1MB 内存，使虚拟地址在逻辑上连续
#define K_HEAP_START 0xc0100000

#define PDE_IDX(addr) (((addr) & 0xffc00000) >> 22) // 高10位
#define PTE_IDX(addr) (((addr) & 0x003ff000) >> 12) // 中10位

// 内存池结构，生成两个实例用于管理内核内存池和用户内存池
struct pool{
    struct bitmap pool_bitmap;  // 本内存池用到的位图结构，用于管理物理内存
    uint32_t phy_addr_start;    // 本内存池所管理物理内存的起始地址
    uint32_t pool_size;         // 本内存池字节容量
};

struct pool kernel_pool, user_pool; // 生成内核内存池和用户内存池
struct virtual_addr kernel_vaddr;   // 次结构用来给内核分配虚拟地址

static void show_memory_size(uint32_t all_mem){
    int name_idx = 0;
    char const* names[] = {
        "B",
        "KB",
        "MB",
        "GB",
    };
    uint32_t new_size = all_mem;
    put_str("            total_memory:0x"); 

    while (new_size > 0)
    {
        new_size >>= 10;
        if(new_size <= 0)
            break;
        
        ++name_idx;
        all_mem = new_size;        
    } 
    
    put_int(all_mem); put_str(names[name_idx]); put_char('\n');
}

// 初始化内存池
static void mem_pool_init(uint32_t all_mem){
    put_str("        mem_pool_init start\n");
    show_memory_size(all_mem);

    uint32_t page_table_size = PG_SIZE * 256; // 页表大小 = 1页的页目录表 + 第0和第768个页表项指向同一个页表 + 第 769 ~ 1022 个页目录项共指向254个页表，共256个页框=1MB
    uint32_t used_mem = page_table_size + 0x100000; // 低端1MB内存
    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_pages = free_mem / PG_SIZE;
    uint16_t kernel_free_pages = all_free_pages / 2;
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;
    // 为简化位图操作，余数不处理，坏处是这样会丢内存
    // 好处是不用做内存的越界检查，因为位图表示的内存不少于实际物理内存
    uint32_t kbm_length = kernel_free_pages / 8;    // 内核位图需要用多少字节
    uint32_t ubm_length = user_free_pages / 8;  // 用户位图需要用多少字节
    uint32_t kp_start = used_mem;// kernel pool start 内核内存池的起始地址
    uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;
    
    kernel_pool.phy_addr_start = kp_start;
    kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
    kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;

    user_pool.phy_addr_start = up_start;
    user_pool.pool_size = user_free_pages * PG_SIZE;
    user_pool.pool_bitmap.btmp_bytes_len = ubm_length;

    // 内核内存池和用户内存池位图
    // 全局或静态的数组需要在编译时知道其长度
    // 而我们需要根据总内存大小算出需要多少字节
    // 所以改为指定一块内存来生成位图

    // 内核使用的最高地址是 0xc009f000, 这是主线程的栈地址(内核的大小预计为70K左右)
    // 32M 内存占用的位图是32MB/4KB/8=1K. 内核内存池的位图先定在 MEM_BITMAP_BASE(0xc009a000)

    kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;
    user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length); // 在内核内存池位图之后
    // 输出内存池信息
    put_str("            kernel_pool_bitmap_start  :"); put_int((int)kernel_pool.pool_bitmap.bits); put_char('\n');
    put_str("            kernel_pool_bitmap_end    :"); put_int((int)kernel_pool.pool_bitmap.bits + kernel_pool.pool_bitmap.btmp_bytes_len); put_char('\n');
    put_str("            kernel_pool_phy_addr_start:"); put_int(kernel_pool.phy_addr_start); put_char('\n');
    put_str("            kernel_pool_phy_addr_end  :"); put_int(kernel_pool.phy_addr_start + kernel_pool.pool_size); put_str("\n\n");
    
    put_str("            user_pool_bitmap_start  :"); put_int((int)user_pool.pool_bitmap.bits); put_char('\n');
    put_str("            user_pool_bitmap_end    :"); put_int((int)user_pool.pool_bitmap.bits + user_pool.pool_bitmap.btmp_bytes_len); put_char('\n');
    put_str("            user_pool_phy_addr_start:"); put_int(user_pool.phy_addr_start); put_char('\n');
    put_str("            user_pool_phy_addr_end  :"); put_int(user_pool.phy_addr_start + user_pool.pool_size); put_char('\n');

    // 将位图置零
    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);

    // 下面初始化内核虚拟地址的位图，按实际物理内存大小生成数组
    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;

    // 位图的数组指向一块未使用的内存，目前定位在内核内存池和用户内存池之外
    kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);

    kernel_vaddr.vaddr_start = K_HEAP_START;
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    put_str("        mem_pool_init done\n");
}

// 分配pg_cnt个虚拟页
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt){
    int vaddr_start = 0, bit_idx_start = -1;
    uint32_t cnt = 0;
    if(PF_KERNEL == pf){
        bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
        if(-1 == bit_idx_start)
            return NULL;
        while (cnt < pg_cnt)
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
        vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
    }else{
        // 用户内存池，将来实现用户进程再补充
    }
    return (void*)vaddr_start;
}

void mem_init(void){
    put_str("    mem_init start \n");
    uint32_t mem_bytes_total = *(uint32_t*)0xb00;   // loader 里面获取
    mem_pool_init(mem_bytes_total); // 初始化内存池
    put_str("    mem_init done \n");
}

