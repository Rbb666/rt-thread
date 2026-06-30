/*
 * assert_test.c
 *
 * 断言失败测试: 触发 RT-Thread 的 RT_ASSERT(0)。
 *
 * RT-Thread 的 RT_ASSERT 在 RT_DEBUGING_ASSERT=y 时会调
 * rt_assert_handler(ex, func, line), 默认实现是 rt_kprintf 打印 function/line
 * 后死循环 -- 比触发 HardFault 温和, 能直接看到出错的源位置, 适合调试。
 *
 * MSH command: assert_test
 */
#include <rtthread.h>
#include "fault_test_common.h"

static void assert_test(int argc, char **argv)
{
    rt_kprintf("\n[assert_test] will trigger RT_ASSERT(0) in %d ms\n",
               FAULT_TEST_DELAY_MS);
    rt_thread_mdelay(FAULT_TEST_DELAY_MS);

    /* 触发断言: 默认实现 = 打印 (file/func/line) + 死循环。 */
    RT_ASSERT(0);

    /* 不会到这里。 */
    rt_kprintf("unreachable\n");
}
MSH_CMD_EXPORT(assert_test, trigger assert to exercise the assert path);