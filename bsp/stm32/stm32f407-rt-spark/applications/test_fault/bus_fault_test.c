/*
 * bus_fault_test.c
 *
 * 总线异常测试: 访问未连接的外部总线地址 0xA0000000 (FSMC bank3)。
 *
 * STM32F407 的 FSMC bank3 落在 0xA0000000-0xAFFFFFFF。本 BSP 没初始化
 * FSMC，DCODE 总线读这里返回 BUSERROR, 内核登记为 Precise BusFault
 * (CFSR.BFSR.PRECISERR=1)，BFAR 会被填上 0xA0000000。Cortex-M 还会把它
 * 升级为 HardFault (BUSFAULTACT + HARDFAULTACT)。
 *
 * 区分点: bus_fault_test 是"主动探测未实现外设"，多见于硬件 bring-up。
 *
 * MSH command: bus_fault_test
 */
#include <rtthread.h>
#include <board.h>
#include "fault_test_common.h"

/* FSMC bank3 地址窗: 本 BSP 未接外部设备, 一读就 BusFault。 */
#define BAD_BUS_ADDR            ((volatile uint32_t *)0xD0000000UL)

static void bus_fault_test(int argc, char **argv)
{
    FAULT_TEST_WARN("bus_fault_test", "read unmapped FSMC address 0xD0000000 (BusFault)");

    rt_thread_mdelay(FAULT_TEST_DELAY_MS);

    volatile uint32_t *bad = BAD_BUS_ADDR;
    uint32_t v = *bad;   /* 触发 Precise BusFault */
    (void)v;

    rt_kprintf("unreachable, v=0x%08X\n", v);
}
MSH_CMD_EXPORT(bus_fault_test, read 0xD0000000 to trigger BusFault);