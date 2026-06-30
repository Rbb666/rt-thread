/*
 * stack_overflow_test.c
 *
 * 栈溢出测试: 在 MSH 线程里无限递归, 每层多吃 256 字节栈。
 *
 * tshell 的 FINSH_THREAD_STACK_SIZE=4096, 4096/256 ~ 16 层后会撞到
 * 栈底 (下一个变量区或链接器 guard), 后续压栈要么:
 *   - 踩到相邻线程的栈/控制块, 引发各种诡异 fault;
 *   - 若 CCR.STKOFEN=1 还会先触发 STKOF UsageFault, 但 RT-Thread
 *     通常没开 STKOFEN, 所以多数情况直接走 HardFault。
 *
 * 注意: 在 MSH 线程里跑, 触发后会拖死 MSH, 串口也不会再有反应, 需硬件复位。
 *
 * MSH command: stack_overflow_test
 */
#include <rtthread.h>
#include "fault_test_common.h"

/* 每层吃掉 256 字节: 触发快且稳定。 */
#define SOF_FRAME_BYTES         256

static void stack_overflow_recurse(int depth)
{
    /* volatile: 让编译器无法把 buf 优化掉, 保证栈确实被消耗。 */
    volatile uint8_t buf[SOF_FRAME_BYTES];
    for (int i = 0; i < SOF_FRAME_BYTES; i++) {
        buf[i] = (uint8_t)(depth + i);
    }

    if (depth == 1) {
        rt_kprintf("[stack_overflow] recursing, frame=%d bytes, "
                   "MSH stack ~4096B -> overflow in ~16 frames\n",
                   SOF_FRAME_BYTES);
    }
    if ((depth % 4) == 0) {
        rt_kprintf("[stack_overflow] depth=%d, sp walking down\n", depth);
    }

    /* 不设终止条件 -- 一直递归直到栈爆。 */
    stack_overflow_recurse(depth + 1);
}

static void stack_overflow_test(int argc, char **argv)
{
    FAULT_TEST_WARN("stack_overflow_test", "infinite recursion to overflow MSH stack");

    rt_thread_mdelay(FAULT_TEST_DELAY_MS);

    stack_overflow_recurse(1);

    /* 不可达。 */
    rt_kprintf("unreachable\n");
}
MSH_CMD_EXPORT(stack_overflow_test, recurse to overflow the calling (MSH) thread stack);