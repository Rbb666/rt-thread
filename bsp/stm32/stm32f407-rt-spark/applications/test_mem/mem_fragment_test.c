/*
 * mem_fragment_test.c
 *
 * MSH 命令:
 *   mem_frag_test
 *
 * 通过对比 rt_malloc() 和 rt_mempool，演示固定大小对象使用内存池
 * 可以降低外部碎片带来的分配失败风险。
 */
#include <rtthread.h>

/* malloc 碎片测试：交错申请小块和较大块，再释放较大块制造不连续空洞。 */
#define MEM_FRAG_PAIR_MAX        192
#define MEM_FRAG_SMALL_SIZE      64
#define MEM_FRAG_HOLE_SIZE       512
#define MEM_FRAG_BIG_REQ_SIZE    2048

/* mempool 对照测试：固定 256 字节 block，释放后可直接回到空闲链表复用。 */
#define MEM_FRAG_MP_BLOCK_SIZE   256
#define MEM_FRAG_MP_BLOCK_COUNT  8
#define MEM_FRAG_MP_POOL_SIZE    ((MEM_FRAG_MP_BLOCK_SIZE + sizeof(rt_uint8_t *)) * MEM_FRAG_MP_BLOCK_COUNT)

/* 保存 malloc 返回的指针，便于测试结束时统一释放，避免示例本身泄漏内存。 */
static void *mem_frag_small_blocks[MEM_FRAG_PAIR_MAX];
static void *mem_frag_hole_blocks[MEM_FRAG_PAIR_MAX];

/* rt_mp_init() 会在池内写指针链表，池起始地址需要按系统对齐要求放置。 */
rt_align(RT_ALIGN_SIZE)
static rt_uint8_t mem_frag_mp_pool[MEM_FRAG_MP_POOL_SIZE];

static void mem_frag_clear_records(void)
{
    int i;

    for (i = 0; i < MEM_FRAG_PAIR_MAX; i++)
    {
        mem_frag_small_blocks[i] = RT_NULL;
        mem_frag_hole_blocks[i] = RT_NULL;
    }
}

static void mem_frag_malloc_case(void)
{
    int i;
    int small_count;
    int hole_count;
    void *probe;

    rt_kprintf("\n[case1] rt_malloc mixed-size fragmentation demo\n");
    rt_kprintf("[case1] pattern: 64, 512, 64, 512 ... until malloc fails\n");

    mem_frag_clear_records();
    small_count = 0;
    hole_count = 0;

    /*
     * 尽量把 heap 填到 rt_malloc() 失败，减少尾部仍存在大块连续空闲区的概率。
     * 分配顺序为： 64 / 512 / 64 / 512 ...
     */
    for (i = 0; i < MEM_FRAG_PAIR_MAX; i++)
    {
        mem_frag_small_blocks[small_count] = rt_malloc(MEM_FRAG_SMALL_SIZE);
        if (mem_frag_small_blocks[small_count] == RT_NULL)
        {
            break;
        }
        rt_memset(mem_frag_small_blocks[small_count], 0x11, MEM_FRAG_SMALL_SIZE);
        small_count++;

        mem_frag_hole_blocks[hole_count] = rt_malloc(MEM_FRAG_HOLE_SIZE);
        if (mem_frag_hole_blocks[hole_count] == RT_NULL)
        {
            break;
        }
        rt_memset(mem_frag_hole_blocks[hole_count], 0x22, MEM_FRAG_HOLE_SIZE);
        hole_count++;
    }

    rt_kprintf("[case1] allocated %d small blocks and %d 512-byte blocks\n",
               small_count,
               hole_count);

    /*
     * 只释放中间的 512 字节块，保留 64 字节块。
     * 这样 heap 中会出现很多被小块隔开的空洞，空闲总量增加，
     * 但连续空闲块不一定足够大。
     */
    for (i = 0; i < hole_count; i++)
    {
        rt_free(mem_frag_hole_blocks[i]);
        mem_frag_hole_blocks[i] = RT_NULL;
    }

    rt_kprintf("[case1] freed all 512-byte blocks, kept 64-byte blocks\n");
    rt_kprintf("[case1] total freed hole bytes about %d\n", hole_count * MEM_FRAG_HOLE_SIZE);

    /* 申请一个大于单个空洞的连续块（2KB），用来观察外部碎片带来的影响。 */
    probe = rt_malloc(MEM_FRAG_BIG_REQ_SIZE);
    if (probe == RT_NULL)
    {
        rt_kprintf("[case1] rt_malloc(%d) failed: free space is split into small holes\n",
                   MEM_FRAG_BIG_REQ_SIZE);
    }
    else
    {
        rt_kprintf("[case1] rt_malloc(%d) OK: heap still had one contiguous block\n",
                   MEM_FRAG_BIG_REQ_SIZE);
        rt_free(probe);
    }

    /* 清理仍保留的小块，避免本测试影响后续其它用例。 */
    for (i = 0; i < small_count; i++)
    {
        rt_free(mem_frag_small_blocks[i]);
        mem_frag_small_blocks[i] = RT_NULL;
    }

    rt_kprintf("[case1] cleanup: freed remaining small blocks\n");
}

static void mem_frag_mempool_case(void)
{
    int i;
    int reused;
    struct rt_mempool mp;
    void *blocks[MEM_FRAG_MP_BLOCK_COUNT];
    void *reuse_blocks[MEM_FRAG_MP_BLOCK_COUNT / 2];

    rt_kprintf("\n[case2] rt_mempool fixed-size block demo\n");

    /* 记录已分配的 block，便于最后成对释放。 */
    for (i = 0; i < MEM_FRAG_MP_BLOCK_COUNT; i++)
    {
        blocks[i] = RT_NULL;
    }
    for (i = 0; i < (MEM_FRAG_MP_BLOCK_COUNT / 2); i++)
    {
        reuse_blocks[i] = RT_NULL;
    }

    /*
     * 使用静态数组初始化内存池，不从系统 heap 申请内存。
     */
    if (rt_mp_init(&mp,
                   "mfrag",
                   mem_frag_mp_pool,
                   sizeof(mem_frag_mp_pool),
                   MEM_FRAG_MP_BLOCK_SIZE) != RT_EOK)
    {
        rt_kprintf("[case2] mempool init failed\n");
        return;
    }

    /* 先把固定块全部申请出来，模拟内存池被用满。 */
    for (i = 0; i < MEM_FRAG_MP_BLOCK_COUNT; i++)
    {
        blocks[i] = rt_mp_alloc(&mp, RT_WAITING_NO);
    }

    rt_kprintf("[case2] allocated %d fixed blocks, free count %d\n",
               MEM_FRAG_MP_BLOCK_COUNT,
               mp.block_free_count);

    /* 释放一半 block，这些 block 会直接回到 mempool 的空闲链表。 */
    for (i = 0; i < MEM_FRAG_MP_BLOCK_COUNT; i += 2)
    {
        rt_mp_free(blocks[i]);
        blocks[i] = RT_NULL;
    }

    rt_kprintf("[case2] freed every other block, free count %d\n", mp.block_free_count);

    reused = 0;
    /* 再次申请同样大小的 block，只要空闲链表有节点，就可以稳定复用。 */
    for (i = 0; i < (MEM_FRAG_MP_BLOCK_COUNT / 2); i++)
    {
        reuse_blocks[i] = rt_mp_alloc(&mp, RT_WAITING_NO);
        if (reuse_blocks[i] != RT_NULL)
        {
            reused++;
        }
    }

    rt_kprintf("[case2] re-allocated %d blocks from mempool, free count %d\n",
               reused,
               mp.block_free_count);
    rt_kprintf("[case2] fixed-size pool reuses blocks directly, no external fragmentation\n");

    /* 释放重新申请的 block。 */
    for (i = 0; i < (MEM_FRAG_MP_BLOCK_COUNT / 2); i++)
    {
        if (reuse_blocks[i] != RT_NULL)
        {
            rt_mp_free(reuse_blocks[i]);
            reuse_blocks[i] = RT_NULL;
        }
    }

    /* 释放测试中仍持有的原始 block。 */
    for (i = 0; i < MEM_FRAG_MP_BLOCK_COUNT; i++)
    {
        if (blocks[i] != RT_NULL)
        {
            rt_mp_free(blocks[i]);
            blocks[i] = RT_NULL;
        }
    }

    /* 静态内存池使用 rt_mp_detach() 退出对象管理，不释放静态数组本身。 */
    rt_mp_detach(&mp);
}

static void mem_frag_test(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    rt_kprintf("\n[mem_frag_test] start\n");

    mem_frag_malloc_case();
    mem_frag_mempool_case();

    rt_kprintf("\n[summary]\n");
    rt_kprintf("  malloc : flexible sizes, but mixed lifetime blocks may split heap\n");
    rt_kprintf("  mempool: fixed block size, less flexible, stable for frequent same-size objects\n");
    rt_kprintf("[mem_frag_test] end\n");
}
MSH_CMD_EXPORT(mem_frag_test, show malloc fragmentation and mempool behavior);
