/*
 * fault_test_common.h
 *
 * applications/ 下异常注入测试 .c 文件的公共头。
 * 目前只放警告横幅宏 + 默认延时。
 *
 * Copyright (c) 2024, RT-Thread Development Team
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef APPLICATIONS_FAULT_TEST_COMMON_H__
#define APPLICATIONS_FAULT_TEST_COMMON_H__

#include <rtthread.h>

/*
 * FAULT_TEST_WARN(test_name, fault_msg)
 *
 * 在真正触发异常前打印横幅。提醒用户:
 *   - 准备 UART/RTT 抓日志
 *   - 系统接下来会停住 (HardFault / assert / 死循环)
 *   - 后面默认会延时 2s
 */
#define FAULT_TEST_WARN(test_name, fault_msg)                                            \
    do {                                                                                 \
        rt_kprintf("\n");                                                                \
        rt_kprintf("============================================================\n");  \
        rt_kprintf("  [%s] %s\n", (test_name), (fault_msg));                              \
        rt_kprintf("  Will trigger in ~2s; capture the UART / RTT log first.\n");        \
        rt_kprintf("  System will halt afterwards -- reset required.\n");                \
        rt_kprintf("============================================================\n");  \
    } while (0)

/* 默认的"看清提示"延时 (ms)，各测试 handler 在警告后调用 rt_thread_mdelay 用。 */
#define FAULT_TEST_DELAY_MS      (2000)

#endif /* APPLICATIONS_FAULT_TEST_COMMON_H__ */