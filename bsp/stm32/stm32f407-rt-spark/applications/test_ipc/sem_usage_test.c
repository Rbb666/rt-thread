/*
 * sem_usage_test.c
 *
 * MSH command:
 *   sem_usage_test
 *
 * Demonstrates three semaphore patterns:
 * 1. Bounded resource counting: take before release.
 * 2. Unbounded release is not the resource-counting pattern.
 * 3. A zero-initialized semaphore can notify one event.
 */
#include <rtthread.h>

#define SEM_USAGE_WAIT_TICK 10

static void sem_usage_print_result(const char *tag, rt_err_t result)
{
    if (result == RT_EOK)
    {
        rt_kprintf("%s OK\n", tag);
    }
    else if (result == -RT_ETIMEOUT)
    {
        rt_kprintf("%s timeout\n", tag);
    }
    else if (result == -RT_EFULL)
    {
        rt_kprintf("%s full\n", tag);
    }
    else
    {
        rt_kprintf("%s error %d\n", tag, result);
    }
}

static void sem_usage_case_resource(void)
{
    struct rt_semaphore sem;

    rt_kprintf("\n[case1] resource semaphore: init value = 2\n");
    if (rt_sem_init(&sem, "sem_r", 2, RT_IPC_FLAG_PRIO) != RT_EOK)
    {
        rt_kprintf("[case1] init failed\n");
        return;
    }

    sem_usage_print_result("[case1] take #1", rt_sem_take(&sem, SEM_USAGE_WAIT_TICK));
    sem_usage_print_result("[case1] take #2", rt_sem_take(&sem, SEM_USAGE_WAIT_TICK));
    sem_usage_print_result("[case1] take #3", rt_sem_take(&sem, SEM_USAGE_WAIT_TICK));
    sem_usage_print_result("[case1] release #1", rt_sem_release(&sem));
    sem_usage_print_result("[case1] release #2", rt_sem_release(&sem));

    rt_kprintf("[case1] rule: for resources, take and release should be paired\n");
    rt_sem_detach(&sem);
}

static void sem_usage_case_over_release(void)
{
    struct rt_semaphore sem;

    rt_kprintf("\n[case2] over-release demo: init value = 1\n");
    if (rt_sem_init(&sem, "sem_o", 1, RT_IPC_FLAG_PRIO) != RT_EOK)
    {
        rt_kprintf("[case2] init failed\n");
        return;
    }

    sem_usage_print_result("[case2] release #1 without take", rt_sem_release(&sem));
    sem_usage_print_result("[case2] release #2 without take", rt_sem_release(&sem));
    sem_usage_print_result("[case2] take #1", rt_sem_take(&sem, SEM_USAGE_WAIT_TICK));
    sem_usage_print_result("[case2] take #2", rt_sem_take(&sem, SEM_USAGE_WAIT_TICK));
    sem_usage_print_result("[case2] take #3", rt_sem_take(&sem, SEM_USAGE_WAIT_TICK));
    sem_usage_print_result("[case2] take #4", rt_sem_take(&sem, SEM_USAGE_WAIT_TICK));

    rt_kprintf("[case2] lesson: init value is not a max resource limit\n");
    rt_sem_detach(&sem);
}

static void sem_usage_case_event(void)
{
    struct rt_semaphore sem;

    rt_kprintf("\n[case3] event notification with sem: init value = 0\n");
    if (rt_sem_init(&sem, "sem_e", 0, RT_IPC_FLAG_PRIO) != RT_EOK)
    {
        rt_kprintf("[case3] init failed\n");
        return;
    }

    sem_usage_print_result("[case3] take before event", rt_sem_take(&sem, SEM_USAGE_WAIT_TICK));
    sem_usage_print_result("[case3] release event", rt_sem_release(&sem));
    sem_usage_print_result("[case3] take after event", rt_sem_take(&sem, SEM_USAGE_WAIT_TICK));
    rt_sem_detach(&sem);

    rt_kprintf("[case3] note: for a one-shot done event, see completion_usage_test\n");
}

static void sem_usage_test(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    rt_kprintf("\n[sem_usage_test] start\n");
    sem_usage_case_resource();
    sem_usage_case_over_release();
    sem_usage_case_event();
    rt_kprintf("\n[sem_usage_test] end\n");
}
MSH_CMD_EXPORT(sem_usage_test, show recommended and risky semaphore usage);
