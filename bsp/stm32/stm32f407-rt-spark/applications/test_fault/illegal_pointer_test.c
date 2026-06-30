/*
 * illegal_pointer_test.c
 *
 * 非法指针解引用测试: 模拟代码里的"野指针" bug (例如强转错地址、
 * 数组越界后被当成指针用), 地址看起来"像那么回事"但落到保留区。
 *
 * 选用 0x12345678: 落在 ARM 内存映射的"外部 RAM"区 (0x10000000-0x1FFFFFFF),
 * 本 BSP 没接外部 SRAM, DCODE 总线读这里返回 BUSERROR, 触发 BusFault。
 *
 * 与 bus_fault_test 的区别:
 *   - bus_fault_test   主动探测 0xA0000000 这种"工程上有意义"的地址
 *   - illegal_pointer_test  模拟"程序员写错了"的场景, 0x12345678 看起来
 *                          像是某结构体字段, 但其实没意义
 *
 * MSH command: illegal_pointer_test
 */
#include <rtthread.h>
#include <board.h>
#include "fault_test_common.h"

/* "外部 RAM" 区域里一个看起来像结构体成员地址的位置: 实际未连接, 一读就 BusFault。 */
#define WILD_POINTER_ADDR       ((volatile uint32_t *)0x12345678UL)

static void illegal_pointer_test(int argc, char **argv)
{
    FAULT_TEST_WARN("illegal_pointer_test", "dereference wild pointer 0x12345678 (BusFault)");

    rt_thread_mdelay(FAULT_TEST_DELAY_MS);

    volatile uint32_t *wild = WILD_POINTER_ADDR;
    uint32_t v = *wild;   /* 野指针解引用 */
    (void)v;

    rt_kprintf("unreachable, v=0x%08X\n", v);
}
MSH_CMD_EXPORT(illegal_pointer_test, dereference wild pointer 0x12345678 to trigger BusFault);