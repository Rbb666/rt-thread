/*
 * schedule_yield_test.c
 *
 * MSH command:
 *   schedule_yield_test
 *
 * Demonstrates the difference between rt_thread_yield() and rt_schedule().
 */
#include <rtthread.h>

#define SCHED_YIELD_STACK_SIZE 1024
#define SCHED_YIELD_TICK       10
#define SCHED_YIELD_SAME_PRIO  21
#define SCHED_YIELD_HIGH_PRIO  20
#define SCHED_YIELD_LOW_PRIO   22
#define SCHED_YIELD_LOOP       5

struct sched_yield_ctx
{
    struct rt_semaphore start;
    struct rt_semaphore done;
};

static void sched_yield_wait_start(struct sched_yield_ctx *ctx)
{
    rt_sem_take(&ctx->start, RT_WAITING_FOREVER);
}

static void sched_yield_print_done(struct sched_yield_ctx *ctx)
{
    rt_sem_release(&ctx->done);
}

static void yield_same_entry(void *parameter)
{
    struct sched_yield_ctx *ctx;
    int index;

    ctx = (struct sched_yield_ctx *)parameter;
    sched_yield_wait_start(ctx);

    for (index = 0; index < SCHED_YIELD_LOOP; index++)
    {
        rt_kprintf("[%s] loop %d, call rt_thread_yield()\n",
                   rt_thread_self()->parent.name, index);
        rt_thread_yield();
    }

    sched_yield_print_done(ctx);
}

static void yield_high_entry(void *parameter)
{
    struct sched_yield_ctx *ctx;
    int index;

    ctx = (struct sched_yield_ctx *)parameter;
    sched_yield_wait_start(ctx);

    for (index = 0; index < SCHED_YIELD_LOOP; index++)
    {
        rt_kprintf("[%s] high-priority loop %d, call rt_thread_yield()\n",
                   rt_thread_self()->parent.name, index);
        rt_thread_yield();
    }

    sched_yield_print_done(ctx);
}

static void yield_low_entry(void *parameter)
{
    struct sched_yield_ctx *ctx;
    int index;

    ctx = (struct sched_yield_ctx *)parameter;
    sched_yield_wait_start(ctx);

    for (index = 0; index < SCHED_YIELD_LOOP; index++)
    {
        rt_kprintf("[%s] low-priority loop %d\n",
                   rt_thread_self()->parent.name, index);
        rt_thread_yield();
    }

    sched_yield_print_done(ctx);
}

static void schedule_same_entry(void *parameter)
{
    struct sched_yield_ctx *ctx;
    int index;

    ctx = (struct sched_yield_ctx *)parameter;
    sched_yield_wait_start(ctx);

    for (index = 0; index < SCHED_YIELD_LOOP; index++)
    {
        rt_kprintf("[%s] loop %d, call rt_schedule()\n",
                   rt_thread_self()->parent.name, index);
        rt_schedule();
    }

    sched_yield_print_done(ctx);
}

static void schedule_high_entry(void *parameter)
{
    struct sched_yield_ctx *ctx;
    int index;

    ctx = (struct sched_yield_ctx *)parameter;
    sched_yield_wait_start(ctx);

    for (index = 0; index < SCHED_YIELD_LOOP; index++)
    {
        rt_kprintf("[%s] high-priority loop %d, call rt_schedule()\n",
                   rt_thread_self()->parent.name, index);
        rt_schedule();
    }

    sched_yield_print_done(ctx);
}

static void schedule_low_entry(void *parameter)
{
    struct sched_yield_ctx *ctx;
    int index;

    ctx = (struct sched_yield_ctx *)parameter;
    sched_yield_wait_start(ctx);

    for (index = 0; index < SCHED_YIELD_LOOP; index++)
    {
        rt_kprintf("[%s] low-priority loop %d, call rt_schedule()\n",
                   rt_thread_self()->parent.name, index);
        rt_schedule();
    }

    sched_yield_print_done(ctx);
}

static rt_err_t sched_yield_start_thread(const char *name,
                                         void (*entry)(void *parameter),
                                         void *parameter,
                                         rt_uint8_t priority)
{
    rt_thread_t tid;

    tid = rt_thread_create(name,
                           entry,
                           parameter,
                           SCHED_YIELD_STACK_SIZE,
                           priority,
                           SCHED_YIELD_TICK);
    if (tid == RT_NULL)
    {
        rt_kprintf("[schedule_yield_test] create %s failed\n", name);
        return -RT_ERROR;
    }

    return rt_thread_startup(tid);
}

static void sched_yield_wait_done(struct sched_yield_ctx *ctx, int count)
{
    int index;

    for (index = 0; index < count; index++)
    {
        rt_sem_take(&ctx->done, RT_WAITING_FOREVER);
    }
}

static void sched_yield_release_start(struct sched_yield_ctx *ctx, int count)
{
    int index;

    for (index = 0; index < count; index++)
    {
        rt_sem_release(&ctx->start);
    }
}

static void schedule_yield_case_same_priority_yield(struct sched_yield_ctx *ctx)
{
    rt_kprintf("\n[case1] same priority + rt_thread_yield()\n");
    rt_kprintf("[case1] expected: ya and yb take turns at the same priority\n");

    if (sched_yield_start_thread("ya", yield_same_entry, ctx, SCHED_YIELD_SAME_PRIO) != RT_EOK)
    {
        return;
    }
    if (sched_yield_start_thread("yb", yield_same_entry, ctx, SCHED_YIELD_SAME_PRIO) != RT_EOK)
    {
        sched_yield_release_start(ctx, 1);
        sched_yield_wait_done(ctx, 1);
        return;
    }

    sched_yield_release_start(ctx, 2);
    sched_yield_wait_done(ctx, 2);
}

static void schedule_yield_case_diff_priority_yield(struct sched_yield_ctx *ctx)
{
    rt_kprintf("\n[case2] different priority + high thread calls rt_thread_yield()\n");
    rt_kprintf("[case2] expected: yh keeps running before yl because priority still wins\n");

    if (sched_yield_start_thread("yl", yield_low_entry, ctx, SCHED_YIELD_LOW_PRIO) != RT_EOK)
    {
        return;
    }
    if (sched_yield_start_thread("yh", yield_high_entry, ctx, SCHED_YIELD_HIGH_PRIO) != RT_EOK)
    {
        sched_yield_release_start(ctx, 1);
        sched_yield_wait_done(ctx, 1);
        return;
    }

    sched_yield_release_start(ctx, 2);
    sched_yield_wait_done(ctx, 2);
}

static void schedule_yield_case_same_priority_schedule(struct sched_yield_ctx *ctx)
{
    rt_kprintf("\n[case3] same priority + rt_schedule()\n");
    rt_kprintf("[case3] expected: sa can keep running; rt_schedule() does not mark yield\n");

    if (sched_yield_start_thread("sa", schedule_same_entry, ctx, SCHED_YIELD_SAME_PRIO) != RT_EOK)
    {
        return;
    }
    if (sched_yield_start_thread("sb", schedule_same_entry, ctx, SCHED_YIELD_SAME_PRIO) != RT_EOK)
    {
        sched_yield_release_start(ctx, 1);
        sched_yield_wait_done(ctx, 1);
        return;
    }

    sched_yield_release_start(ctx, 2);
    sched_yield_wait_done(ctx, 2);
}

static void schedule_yield_case_diff_priority_schedule(struct sched_yield_ctx *ctx)
{
    rt_kprintf("\n[case4] different priority + rt_schedule()\n");
    rt_kprintf("[case4] expected: sh keeps running before sl because priority still wins\n");

    if (sched_yield_start_thread("sl", schedule_low_entry, ctx, SCHED_YIELD_LOW_PRIO) != RT_EOK)
    {
        return;
    }
    if (sched_yield_start_thread("sh", schedule_high_entry, ctx, SCHED_YIELD_HIGH_PRIO) != RT_EOK)
    {
        sched_yield_release_start(ctx, 1);
        sched_yield_wait_done(ctx, 1);
        return;
    }

    sched_yield_release_start(ctx, 2);
    sched_yield_wait_done(ctx, 2);
}

static void schedule_yield_test(int argc, char **argv)
{
    struct sched_yield_ctx ctx;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    rt_sem_init(&ctx.start, "syst", 0, RT_IPC_FLAG_PRIO);
    rt_sem_init(&ctx.done, "sydn", 0, RT_IPC_FLAG_PRIO);

    rt_kprintf("\n[schedule_yield_test] start\n");
    schedule_yield_case_same_priority_yield(&ctx);
    schedule_yield_case_diff_priority_yield(&ctx);
    schedule_yield_case_same_priority_schedule(&ctx);
    schedule_yield_case_diff_priority_schedule(&ctx);
    rt_kprintf("\n[schedule_yield_test] summary:\n");
    rt_kprintf("  rt_thread_yield(): mark current thread as yielded, then reschedule\n");
    rt_kprintf("  rt_schedule(): run scheduler selection only; current thread may stay running\n");
    rt_kprintf("  priority rule still applies after yield\n");
    rt_kprintf("[schedule_yield_test] end\n");

    rt_sem_detach(&ctx.done);
    rt_sem_detach(&ctx.start);
}
MSH_CMD_EXPORT(schedule_yield_test, compare rt_thread_yield and rt_schedule);
