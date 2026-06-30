/*
 * memcheck_test.c
 *
 * MSH command:
 *   memcheck_test
 *
 * This command deliberately corrupts one small-memory block header. Run
 * "memcheck" after it to verify that the heap checker reports the bad block.
 * Reset the board after this test because the system heap is left corrupted.
 */
#include <rtthread.h>

struct memcheck_test_item
{
    rt_uintptr_t pool_ptr;
    rt_size_t next;
    rt_size_t prev;
#ifdef RT_USING_MEMTRACE
#ifdef ARCH_CPU_64BIT
    rt_uint8_t thread[8];
#else
    rt_uint8_t thread[4];
#endif
#endif
};

#define MEMCHECK_TEST_SIZE          64
#define MEMCHECK_TEST_USED_FLAG     0x1
#define MEMCHECK_TEST_CORRUPT_MASK  ((rt_uintptr_t)0x20)
#define MEMCHECK_TEST_HEADER_SIZE   RT_ALIGN(sizeof(struct memcheck_test_item), RT_ALIGN_SIZE)

static void memcheck_test(int argc, char **argv)
{
    void *ptr;
    struct memcheck_test_item *item;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    ptr = rt_malloc(MEMCHECK_TEST_SIZE);
    if (ptr == RT_NULL)
    {
        rt_kprintf("[memcheck_test] rt_malloc(%d) failed\n", MEMCHECK_TEST_SIZE);
        return;
    }

    item = (struct memcheck_test_item *)((rt_uint8_t *)ptr - MEMCHECK_TEST_HEADER_SIZE);

    rt_kprintf("[memcheck_test] allocated %d bytes at %p, header %p\n",
               MEMCHECK_TEST_SIZE, ptr, item);
    rt_kprintf("[memcheck_test] corrupt header pool_ptr: 0x%08x -> 0x%08x\n",
               item->pool_ptr,
               (item->pool_ptr ^ MEMCHECK_TEST_CORRUPT_MASK) | MEMCHECK_TEST_USED_FLAG);

    item->pool_ptr = (item->pool_ptr ^ MEMCHECK_TEST_CORRUPT_MASK) | MEMCHECK_TEST_USED_FLAG;

    rt_kprintf("[memcheck_test] run \"memcheck\" now; reset the board after this test\n");
}
MSH_CMD_EXPORT(memcheck_test, corrupt one heap block header for memcheck test);
