/*
 * completion_usage_test.c
 *
 * MSH command:
 *   completion_usage_test
 *
 * Demonstrates completion as a one-shot "async work is done" notification.
 */
#include <rtthread.h>
#include <ipc/completion.h>

#define COMPLETION_USAGE_WORKER_STACK_SIZE 1024
#define COMPLETION_USAGE_WORKER_PRIORITY   20
#define COMPLETION_USAGE_WORKER_TICK       10

struct completion_usage_ctx
{
    struct rt_completion done;
    int result;
};

static void completion_usage_print_result(const char *tag, rt_err_t result)
{
    if (result == RT_EOK)
    {
        rt_kprintf("%s OK\n", tag);
    }
    else if (result == -RT_ETIMEOUT)
    {
        rt_kprintf("%s timeout\n", tag);
    }
    else
    {
        rt_kprintf("%s error %d\n", tag, result);
    }
}

static void completion_usage_worker(void *parameter)
{
    struct completion_usage_ctx *ctx;

    ctx = (struct completion_usage_ctx *)parameter;

    rt_kprintf("[completion_usage_test] worker: start async work\n");
    rt_thread_mdelay(50);
    ctx->result = 1234;
    rt_kprintf("[completion_usage_test] worker: work done, signal completion\n");
    rt_completion_done(&ctx->done);
}

static void completion_usage_case_no_counting(void)
{
    struct rt_completion done;

    rt_kprintf("\n[completion_usage_test] no-counting demo\n");
    rt_completion_init(&done);

    completion_usage_print_result("[completion_usage_test] wakeup #1",
                                  rt_completion_wakeup(&done));
    completion_usage_print_result("[completion_usage_test] wakeup #2",
                                  rt_completion_wakeup(&done));
    completion_usage_print_result("[completion_usage_test] wait #1",
                                  rt_completion_wait(&done, 0));
    completion_usage_print_result("[completion_usage_test] wait #2",
                                  rt_completion_wait(&done, 0));
    rt_kprintf("[completion_usage_test] lesson: completion is a done state, not a counter\n");
}

static void completion_usage_test(int argc, char **argv)
{
    struct completion_usage_ctx ctx;
    rt_thread_t tid;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    rt_kprintf("\n[completion_usage_test] start\n");
    ctx.result = 0;
    rt_completion_init(&ctx.done);

    tid = rt_thread_create("cplt",
                           completion_usage_worker,
                           &ctx,
                           COMPLETION_USAGE_WORKER_STACK_SIZE,
                           COMPLETION_USAGE_WORKER_PRIORITY,
                           COMPLETION_USAGE_WORKER_TICK);
    if (tid == RT_NULL)
    {
        rt_kprintf("[completion_usage_test] create worker failed\n");
        return;
    }

    rt_thread_startup(tid);
    rt_kprintf("[completion_usage_test] main: wait completion\n");
    completion_usage_print_result("[completion_usage_test] main: completion wait",
                                  rt_completion_wait(&ctx.done, RT_WAITING_FOREVER));
    rt_kprintf("[completion_usage_test] main: worker result = %d\n", ctx.result);

    completion_usage_case_no_counting();

    rt_kprintf("[completion_usage_test] end\n");
}
MSH_CMD_EXPORT(completion_usage_test, show completion usage with async worker);
