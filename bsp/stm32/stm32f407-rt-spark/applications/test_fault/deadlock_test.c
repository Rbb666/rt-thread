/*
 * deadlock_test.c
 *
 * 死锁测试: 经典 AB-BA 死锁。
 *   - 线程 A: 先持有 mtx_a, 200ms 后等 mtx_b
 *   - 线程 B: 先持有 mtx_b, 200ms 后等 mtx_a
 *   - 两者互相等对方释放, 最终两个线程都挂在 rt_mutex_take() 上。
 *
 * 跑完后在 msh 里用 list_thread 可以看到 dl_a / dl_b 状态为 "pending"
 * (挂在对方的 mutex 上)。RT-Thread 的 mutex 默认不检测跨线程死锁,
 * 系统不会自动恢复, 需要手动复位。
 *
 * 注意: 这个测试是"无破坏性"的, 不会让系统崩溃, 但 MSH 线程不受影响
 * -- 你可以继续操作 msh, 验证完成后手动 reset。
 *
 * MSH command: deadlock_test
 */
#include <rtthread.h>
#include "fault_test_common.h"

static struct rt_mutex g_mtx_a;
static struct rt_mutex g_mtx_b;
static volatile rt_bool_t g_deadlock_inited = RT_FALSE;

static void dl_thread_a_entry(void *param)
{
    rt_thread_mdelay(100);   /* 和 B 起点对齐 */

    rt_kprintf("[deadlock] A: take mtx_a\n");
    rt_mutex_take(&g_mtx_a, RT_WAITING_FOREVER);

    rt_kprintf("[deadlock] A: holding mtx_a, sleep 200ms then try mtx_b\n");
    rt_thread_mdelay(200);

    rt_kprintf("[deadlock] A: try mtx_b (held by B) -- will block here forever\n");
    rt_mutex_take(&g_mtx_b, RT_WAITING_FOREVER);

    rt_kprintf("[deadlock] A: should NOT reach here\n");
    rt_mutex_release(&g_mtx_b);
    rt_mutex_release(&g_mtx_a);
}

static void dl_thread_b_entry(void *param)
{
    rt_thread_mdelay(100);

    rt_kprintf("[deadlock] B: take mtx_b\n");
    rt_mutex_take(&g_mtx_b, RT_WAITING_FOREVER);

    rt_kprintf("[deadlock] B: holding mtx_b, sleep 200ms then try mtx_a\n");
    rt_thread_mdelay(200);

    rt_kprintf("[deadlock] B: try mtx_a (held by A) -- will block here forever\n");
    rt_mutex_take(&g_mtx_a, RT_WAITING_FOREVER);

    rt_kprintf("[deadlock] B: should NOT reach here\n");
    rt_mutex_release(&g_mtx_a);
    rt_mutex_release(&g_mtx_b);
}

static void deadlock_test(int argc, char **argv)
{
    if (g_deadlock_inited) {
        rt_kprintf("[deadlock] already started -- use `list_thread` to inspect\n");
        return;
    }

    rt_mutex_init(&g_mtx_a, "dl_mtx_a", RT_IPC_FLAG_PRIO);
    rt_mutex_init(&g_mtx_b, "dl_mtx_b", RT_IPC_FLAG_PRIO);
    g_deadlock_inited = RT_TRUE;

    rt_thread_t ta = rt_thread_create("dl_a", dl_thread_a_entry, RT_NULL,
                                      1024, 10, 10);
    rt_thread_t tb = rt_thread_create("dl_b", dl_thread_b_entry, RT_NULL,
                                      1024, 10, 10);
    if (ta == RT_NULL || tb == RT_NULL) {
        rt_kprintf("[deadlock] thread create failed\n");
        return;
    }
    rt_thread_startup(ta);
    rt_thread_startup(tb);

    rt_kprintf("[deadlock] A and B started; should deadlock in ~0.5s\n");
    rt_kprintf("[deadlock] inspect with `list_thread` "
               "(both will be pending on the other's mutex)\n");
}
MSH_CMD_EXPORT(deadlock_test, two threads acquire mutexes in opposite order to deadlock);