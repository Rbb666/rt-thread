/*
 * div_zero_test.c
 *
 * 整数除零异常测试。
 *
 * 背景: Cortex-M4 的 CCR.DIV_0_TRP 默认是 0 -- 此时 SDIV r0,r1,r2 在 r2==0
 * 时只是安静地把结果设成 0，不进 UsageFault。所以本测试先置位 DIV_0_TRP
 * 再做 /0，让 UsageFault.CFSR.UFSR.DIVBYZERO 真的发生。
 *
 * CmBacktrace (启用中) 会捕获并打印 UsageFault + 调用栈。
 *
 * MSH command: div_zero_test
 */
#include <rtthread.h>
#include <board.h>
#include "fault_test_common.h"

/* SCB->CCR 的 DIV_0_TRP (bit 4)。直接位运算避免再依赖 CMSIS 命名。 */
#define CCR_DIV_0_TRP_BIT       (1UL << 4)

/* volatile 防止编译器把 0 当常量折出去。 */
static volatile int g_div_dividend = 100;
static volatile int g_div_divisor  = 0;

static void div_zero_test(int argc, char **argv)
{
    FAULT_TEST_WARN("div_zero_test", "integer division by zero (UsageFault)");

    /* 留 2s 给用户看清提示、连好抓日志的工具。 */
    rt_thread_mdelay(FAULT_TEST_DELAY_MS);

    /* 置位 DIV_0_TRP: 此后 SDIV 在除数为 0 时产生 UsageFault。 */
    SCB->CCR |= CCR_DIV_0_TRP_BIT;

    /* volatile 读 + 真正执行的 SDIV -- 触发 UsageFault 走 CmBacktrace。 */
    int r = g_div_dividend / g_div_divisor;
    (void)r;

    /* 不会到这里。 */
    rt_kprintf("unreachable, r=%d\n", r);
}
MSH_CMD_EXPORT(div_zero_test, trigger integer division-by-zero (UsageFault));