/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2021-10-17     Meco Man      First version
 * 2022-05-10     Meco Man      improve rt-thread initialization process
 */
#include "../../../packages/benchmark/lv_demo_benchmark.h"

void lv_user_gui_init(void)
{
    /* display demo; you may replace with your LVGL application at here */
//    extern void lv_demo_music(void);
//    lv_demo_music();

    extern void lv_avi_create(void);
    lv_avi_create();
//    extern void lv_demo_benchmark_run_scene(lv_demo_benchmark_mode_t _mode, uint16_t scene_no);
//    lv_demo_benchmark_run_scene(LV_DEMO_BENCHMARK_MODE_RENDER_AND_DRIVER, 26*2-1);

//    extern void lv_demo_benchmark(lv_demo_benchmark_mode_t _mode);
//    lv_demo_benchmark(LV_DEMO_BENCHMARK_MODE_RENDER_AND_DRIVER);
}
