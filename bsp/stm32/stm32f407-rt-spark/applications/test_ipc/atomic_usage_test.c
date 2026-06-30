/*
 * atomic_usage_test.c
 *
 * MSH command:
 *   atomic_usage_test
 *
 * Shows the typical usage difference between atomic operations,
 * semaphore, and mutex when protecting shared state.
 */
#include <rtthread.h>
#include "rtatomic.h"

static void atomic_usage_case_atomic(void)
{
    rt_atomic_t counter;
    rt_atomic_t old_value;
    rt_atomic_t now_value;

    rt_kprintf("\n[case1] atomic protects one shared word\n");

    rt_atomic_store(&counter, 0);
    old_value = rt_atomic_add(&counter, 1);
    now_value = rt_atomic_load(&counter);

    rt_kprintf("[case1] old value %d, current value %d\n",
               (int)old_value,
               (int)now_value);
    rt_kprintf("[case1] usage: counter, flag, reference count\n");
}

static void atomic_usage_case_sem(void)
{
    struct rt_semaphore sem;
    rt_err_t result;

    rt_kprintf("\n[case2] semaphore protects resource count\n");

    if (rt_sem_init(&sem, "asem", 1, RT_IPC_FLAG_PRIO) != RT_EOK)
    {
        rt_kprintf("[case2] sem init failed\n");
        return;
    }

    result = rt_sem_take(&sem, RT_WAITING_NO);
    if (result == RT_EOK)
    {
        rt_kprintf("[case2] take one resource OK\n");
        rt_kprintf("[case2] use the resource here\n");
        rt_sem_release(&sem);
        rt_kprintf("[case2] release the resource OK\n");
    }
    else
    {
        rt_kprintf("[case2] take resource failed: %d\n", result);
    }

    rt_kprintf("[case2] usage: resource pool, producer-consumer token\n");
    rt_sem_detach(&sem);
}

static void atomic_usage_case_mutex(void)
{
    struct rt_mutex mutex;
    int shared_a;
    int shared_b;

    rt_kprintf("\n[case3] mutex protects a critical section\n");

    if (rt_mutex_init(&mutex, "amtx", RT_IPC_FLAG_PRIO) != RT_EOK)
    {
        rt_kprintf("[case3] mutex init failed\n");
        return;
    }

    shared_a = 0;
    shared_b = 0;

    rt_mutex_take(&mutex, RT_WAITING_FOREVER);
    shared_a++;
    shared_b = shared_a * 2;
    rt_mutex_release(&mutex);

    rt_kprintf("[case3] shared_a %d, shared_b %d\n", shared_a, shared_b);
    rt_kprintf("[case3] usage: protect multiple variables or multi-step logic\n");

    rt_mutex_detach(&mutex);
}

static void atomic_usage_test(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    rt_kprintf("\n[atomic_usage_test] start\n");

    atomic_usage_case_atomic();
    atomic_usage_case_sem();
    atomic_usage_case_mutex();

    rt_kprintf("\n[summary]\n");
    rt_kprintf("  atomic: no blocking, one simple shared variable operation\n");
    rt_kprintf("  sem   : count available resources or event tokens\n");
    rt_kprintf("  mutex : lock a critical section, may block and schedule\n");
    rt_kprintf("[atomic_usage_test] end\n");
}
MSH_CMD_EXPORT(atomic_usage_test, show atomic semaphore and mutex usage);
