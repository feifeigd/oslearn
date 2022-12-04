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

// 得到虚拟地址vaddr对应的pte指针
uint32_t* pte_ptr(uint32_t vaddr){
    // 先访问到页表自己 + 再用页表项pde作为pte的索引访问到页表 + 再用pte的索引作为页内便宜
    uint32_t* pte = (uint32_t*)(0xffc00000 | ((vaddr & 0xffc00000) >> 10) | PTE_IDX(vaddr) * 4);
    return pte;
}

// 得到虚拟地址vaddr对应的pde的指针
uint32_t* pde_ptr(uint32_t vaddr){
    // 0xfffff 用来访问到页表本身所在的地址
    uint32_t* pde = (uint32_t*)((0xfffff000) | PDE_IDX(vaddr) * 4);
    return pde;
}

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

// 在m_pool指向的物理内存池中分配一个物理页
// 成功返回页框的物理地址，失败则返回NULL
static void* palloc(struct  pool* m_pool)
{
    // 扫描或设置位图要保证原子操作
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1); // 找一个物理页面
    if(-1 == bit_idx)
        return NULL;
    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
    uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
    return (void*)page_phyaddr;
}

// 页表中添加虚拟地址_vaddr 与物理地址_page_phyaddr的映射
static void page_table_add(void* _vaddr, void* _page_phyaddr){
    uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
    uint32_t* pde = pde_ptr(vaddr);
    uint32_t* pte = pte_ptr(vaddr);
    // 注意
    // 执行 *pte，会访问到空的pde，所以确保pde创建完成后才执行 *pte
    // 否则会引起page_fault。因此在*pde为0时，*pte只能出现在下面else语句块中的*pde后面

    // 先在页目录项内判断目录项的P位，若为1，则表示该页已存在
    if(*pde & 1){  // 页目录项和页表项的第0位为P，此处判断目录项是否存在
        ASSERT(!(*pte & 1));
        if(*pte & 1){ // 只要是创建页表，pte就应该不存在，多判断一下放心
            PANIC("pte repeat");
        }
    }else{  // 页目录项不存在，所以要先创建页目录再创建页表项
        // 页表中用到的页框一律从内核空间分配
        uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
        put_int(pde_phyaddr);put_str("\n");
        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        put_int(*pde);put_str("\n");
        // 页表清零
        memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);
        ASSERT(!(*pte & 1));
    }
    *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
}

// 分配pg_cnt个页空间，成功则返回起始虚拟地址，失败返回NULL
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt){
    ASSERT(pg_cnt > 0 && pg_cnt < 3840);
    // malloc_page 的原理是3个动作合成
    // 1. 通过vaddr_get 在虚拟内存池中申请虚拟地址
    // 2. 通过palloc在物理内存池中申请物理页
    // 3. 通过page_table_add 将以上得到的虚拟地址和物理地址在页表中完成映射
    void* vaddr_start = vaddr_get(pf, pg_cnt);
    if(!vaddr_start)
        return NULL;
    uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
    struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
    // 因为虚拟地址是连续的，但物理地址可以不连续，所以逐个映射
    while (cnt-- > 0)
    {
        void* page_phyaddr = palloc(mem_pool);
        if(!page_phyaddr)
            return NULL;
        page_table_add((void*)vaddr, page_phyaddr);
        vaddr += PG_SIZE; // 下一个虚拟页
    }
    
    return vaddr_start;
}

void* get_kernel_pages(uint32_t pg_cnt){
    void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
    if(vaddr)
        memset(vaddr, 0, pg_cnt * PG_SIZE);
    return vaddr;
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

    kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;  // x 0xc009a000 或 xp 0x0009a000
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



void mem_init(void){
    put_str("    mem_init start \n");
    uint32_t mem_bytes_total = *(uint32_t*)0xb00;   // loader 里面获取
    mem_pool_init(mem_bytes_total); // 初始化内存池
    put_str("    mem_init done \n");
}

