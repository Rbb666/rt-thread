/*
 * mem_leak_test.c
 *
 * MSH command:
 *   mem_leak_test [size]
 *
 * With RT_USING_MEMTRACE enabled, run "memtrace" after this command.
 * The leaked block should be shown with the thread name "leak".
 */
#include <rtthread.h>
#include <stdlib.h>

#define MEM_LEAK_TEST_DEFAULT_SIZE 128
#define MEM_LEAK_TEST_STACK_SIZE   1024
#define MEM_LEAK_TEST_PRIORITY     20
#define MEM_LEAK_TEST_TICK         10

static void mem_leak_entry(void *parameter)
{
    rt_size_t size = (rt_size_t)parameter;
    void *ptr;

    ptr = rt_malloc(size);
    if (ptr == RT_NULL)
    {
        rt_kprintf("[mem_leak_test] rt_malloc(%d) failed\n", size);
        return;
    }

    rt_memset(ptr, 0xA5, size);
    rt_kprintf("[mem_leak_test] leaked %d bytes at %p\n", size, ptr);
}

static void mem_leak_test(int argc, char **argv)
{
    rt_size_t size = MEM_LEAK_TEST_DEFAULT_SIZE;
    rt_thread_t tid;

    if (argc > 1)
    {
        int value = atoi(argv[1]);

        if (value <= 0)
        {
            rt_kprintf("usage: mem_leak_test [positive_size]\n");
            return;
        }

        size = (rt_size_t)value;
    }

    tid = rt_thread_create("leak",
                           mem_leak_entry,
                           (void *)size,
                           MEM_LEAK_TEST_STACK_SIZE,
                           MEM_LEAK_TEST_PRIORITY,
                           MEM_LEAK_TEST_TICK);
    if (tid == RT_NULL)
    {
        rt_kprintf("[mem_leak_test] create thread failed\n");
        return;
    }

    rt_thread_startup(tid);
}
MSH_CMD_EXPORT(mem_leak_test, allocate memory without free for memtrace test);
