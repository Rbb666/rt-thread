#include <rtthread.h>

// 定义两个连续的全局变量，用来观测“写穿”现象
volatile uint32_t target_variable = 0x11111111;
volatile uint8_t  buffer_overflow[4] = {0};

static void test_wild_write(int argc, char **argv)
{
    rt_kprintf("Before overflow: target_variable = 0x%08X\n", target_variable);

    // 故意越界写入 buffer_overflow，根据链接顺序，可能会踩到相邻的变量
    rt_kprintf("Writing outside buffer bounds...\n");
    for (int i = 0; i < 16; i++)
    {
        buffer_overflow[i] = 0x99; 
    }

    rt_kprintf("After overflow:  target_variable = 0x%08X\n", target_variable);
    rt_kprintf("If target_variable changed, 'wild write' detected!\n");
}
MSH_CMD_EXPORT(test_wild_write, trigger global variable overflow/wild write);