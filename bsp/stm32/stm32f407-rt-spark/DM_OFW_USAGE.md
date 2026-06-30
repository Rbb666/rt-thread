# STM32F407 RT-Spark DM/OFW 使用说明

本文记录本 BSP 上验证 RT-Thread DM 设备框架和 OFW 设备树解析的最小使用过程。后续相关用法继续补充到本文。

## 当前验证内容

- SCons 使用 env 内置 dtc 将 DTS 编译为 DTB，再通过固定的 `board/builtin_fdt.S` 内置到固件，启动后由 `rt_fdt_prefetch()` 和 `rt_fdt_unflatten()` 解析。
- 设备树源文件位于 `board/dts/stm32f407-rt-spark.dts`。
- `/cpus/cpu@0` 是当前 RT-Thread OFW unflatten 必需的系统节点，MCU 场景也需要保留。
- `/soc/gpio` 作为 STM32 GPIO provider，compatible 为 `st,stm32-gpio`。
- `/leds/led0` 通过通用 `gpio-leds` 驱动和 `gpios = <&stm32gpio 92 PIN_ACTIVE_LOW>` 引用 PF12 红色 LED。
- `/gpio_keys/key_up` 通过通用 `components/drivers/input/keyboard/keys-gpio.c` 驱动接入 PC5，按键码为 `KEY_UP`。
- 当前 OFW 输入不依赖 DFS/ROMFS，DTB 直接内置在固件中。

## Windows 下编译

在 PowerShell 中进入 BSP 目录：

```powershell
cd D:\workspace_work\rt-thread-5.0\bsp\stm32\stm32f407-rt-spark
$env:PATH="D:\workspace_work\env-windows\tools\dtc-1.4.7-mingw64;$env:PATH"
$env:RTT_EXEC_PATH="D:\workspace_work\env-windows\tools\gnu_gcc\arm_gcc\mingw\bin"
D:\workspace_work\env-windows\tools\python-3.11.9-amd64\Scripts\scons.exe -j4
```

`D:\workspace_work\env-windows\tools\dtc-1.4.7-mingw64` 必须位于 `PATH` 前面，避免命中 Windows 自带的 `dtc` 命令。

## DTS 修改后如何让 OFW 生效

先区分两个概念：

- `DTS` 是设备树源文件。
- `DTB/FDT` 是 DTS 编译后的二进制设备树，OFW 解析的是它。

当前 BSP 的实际链路是：

```text
DTS -> 预处理临时 DTS -> dtc -> DTB/FDT -> 固件内置 -> rt_fdt_prefetch() -> rt_fdt_unflatten()
```

当前 SCons 会在读取 `board/SConscript` 时每次重新生成 `board/dts/stm32f407-rt-spark.dtb`，随后删除 `build/board/builtin_fdt.o`，强制重新汇编固定的 `board/builtin_fdt.S`，让 `.incbin` 内置最新 DTB 到固件。后续修改 DTS 后，只需要重新执行 SCons 编译，不需要先清理工程。

也可以手动验证 DTS 编译：

```powershell
cd D:\workspace_work\rt-thread-5.0\bsp\stm32\stm32f407-rt-spark
$env:PATH="D:\workspace_work\env-windows\tools\dtc-1.4.7-mingw64;$env:PATH"

D:\workspace_work\env-windows\tools\gnu_gcc\arm_gcc\mingw\bin\arm-none-eabi-gcc.exe `
    -E -P -x assembler-with-cpp `
    -I D:\workspace_work\rt-thread-5.0\components\drivers\include `
    board\dts\stm32f407-rt-spark.dts `
    -o board\dts\stm32f407-rt-spark.dts.tmp

dtc -I dts -O dtb `
    board\dts\stm32f407-rt-spark.dts.tmp `
    -o board\dts\stm32f407-rt-spark.dtb

Remove-Item board\dts\stm32f407-rt-spark.dts.tmp
```

注意：PowerShell 中如果 `dtc -v` 输出的是 Windows Distributed Transaction Coordinator 信息，说明当前命中的不是 device-tree compiler。

`ofw_dts` 打印的是 OFW 从 DTB 反解析出的树，和原始 DTS 文本不一定逐字一致：

- `&label` phandle 引用会变成数字，例如 `<&stm32gpio 92 PIN_ACTIVE_LOW>` 可能显示为 `<0x01 0x5c 0x01>`。
- dtc 可能自动补充 `phandle = <...>` 属性，用于 phandle 引用解析。
- 当前构建没有使用 dtc 的 `-@` 和 `-A` 参数，因此不会额外生成 `__symbols__` 节点，也不会把 label 自动加入 `/aliases`。

## 启动后验证命令

进入 `msh />` 后：

```text
ofw_dts
list_device
list_irq all
ls /dev
echo on /dev/led0
echo off /dev/led0
echo toggle /dev/led0
dm_key_input
```

期望现象：

- `ofw_dts` 能打印根节点、`/cpus/cpu@0`、`/soc/gpio`、`/leds/led0` 和 `/gpio_keys/key_up`。
- `list_device` 能看到通用 LED 设备 `led0`，以及 `gpio-keys` 创建的 `input0`。
- `list_irq all` 当前主要用于查看 OFW/PIC 映射；通用 `keys-gpio.c` 仍使用 legacy pin IRQ API。
- `ls /dev` 能看到 devfs 下的 `led0` 设备节点；当前保持 DFS v1，`input0` 仍主要通过 RT-Thread input handler 验证。
- `echo on/off/toggle /dev/led0` 可通过通用 `gpio-leds` 驱动控制红色 LED。`echo on led0` 会按当前目录下的普通文件打开 `led0`，没有挂载到当前目录时会失败。
- `dm_key_input` 显示 BSP 侧 input handler 是否 attached，以及 `gpio-keys` 上报的 KEY_UP press/release 计数。

## 按键接口和 POSIX/devfs 说明

引入设备树后，上层应用不应该再关心 PC5、EXTI、GPIO provider 或 OFW 节点路径。设备树的作用是描述硬件，让通用 `gpio-keys` 驱动自动绑定并创建标准输入设备；上层应用的理想接口是 `/dev/input0`，通过 `open/read/poll/ioctl` 读取 input event。

当前 BSP 按要求保持 DFS v1。RT-Thread 现有 `components/drivers/input/input_uapi.c` 使用的是 DFS v2 风格的 `struct dfs_file_ops` 签名，并依赖 `generic_dfs_lseek`，所以在不侵入修改通用组件的前提下，暂不启用 `RT_INPUT_UAPI`。因此当前 MSH 的文件系统命令只能用于验证 devfs 和 LED：

- `ls /dev`：确认 `led0` 等 devfs 设备节点。
- `echo on/off/toggle /dev/led0`：通过通用 `gpio-leds` 驱动验证输出设备。
- `dm_key_input`：当前 BSP 侧最小 input handler 验证命令，用来确认 `gpio-keys` 已经收到 PC5 按键事件。

如果后续要把按键完全暴露为 POSIX/devfs 文件接口，有两个方向：

- 保持 DFS v1：需要让通用 `input_uapi.c` 兼容 DFS v1 的 fops 签名，或者新增一层不侵入组件的适配包装。
- 切换 DFS v2：可以直接启用 `RT_INPUT_UAPI`、`RT_USING_POSIX_DEVIO` 和 `RT_USING_CLOCK_TIME`，应用侧读取 `/dev/input0` 的 `struct input_event`。

应用侧目标形态如下：

```c
#include <fcntl.h>
#include <sys/unistd.h>
#include <drivers/input_uapi.h>

int fd = open("/dev/input0", O_RDONLY);
struct input_event ev;

while (read(fd, &ev, sizeof(ev)) == sizeof(ev))
{
    if (ev.type == EV_KEY && ev.code == KEY_UP && ev.value == 1)
    {
        /* key pressed */
    }
}
```

注意：`cat /dev/input0` 即使可打开，也不是合适的按键测试方式。input event 是二进制 `struct input_event`，不是文本流；没有事件时还会阻塞等待按键。

## 设备树 pin 表达

当前 GPIO provider 使用两个 cell：

```dts
stm32gpio: gpio {
    compatible = "st,stm32-gpio";
    gpio-controller;
    #gpio-cells = <2>;
};
```

GPIO consumer 使用：

```dts
gpios = <&stm32gpio 92 PIN_ACTIVE_LOW>;
gpios = <&stm32gpio 37 PIN_PULL_DISABLE>;
```

第一个参数是 RT-Thread STM32 pin 编号，计算方式与 `GET_PIN(PORT, PIN)` 一致：`PF12 = 5 * 16 + 12 = 92`，`PC5 = 2 * 16 + 5 = 37`。第二个参数是 `dt-bindings/pin/pin.h` 中定义的 flags。

PC5 按键检测当前主路径是通用 `gpio-keys`：

```text
DTS gpio_keys/key_up -> keys-gpio.c -> rt_ofw_get_named_pin() -> rt_pin_attach_irq() -> rt_pin_irq_enable()
下降沿中断 -> gpio_key_event() -> rt_input_report_key(KEY_UP) -> BSP input handler 计数
```

注意：当前通用 `keys-gpio.c` 会把 `rt_ofw_get_named_pin()` 返回的 `mode` 直接传给 `rt_pin_attach_irq()`。这个 `mode` 在 OFW pin 语义里本来是 GPIO 输入/上拉/下拉模式，但在 pin IRQ API 里被当成中断触发模式使用。因此当前 DTS 对 `gpio-keys` 使用 `PIN_PULL_DISABLE`，让转换结果为 `PIN_MODE_INPUT = 1`，在 IRQ API 中等价于 `PIN_IRQ_MODE_FALLING = 1`。STM32 底层在 falling EXTI 模式下会配置为 pull-up。

这个写法是为了不侵入修改 `components/drivers/input/keyboard/keys-gpio.c` 的临时适配。后续更合理的方向是让通用 `keys-gpio.c` 支持单独的 IRQ 触发模式属性，或由 pinctrl 独立描述默认上拉输入状态。

## STM32 OFW PIC / interrupt-controller 适配现状

当前 STM32 GPIO 对接 OFW PIC 的最小闭环是可行的，已经拆成三层：

- 设备树中 GPIO provider 增加 `interrupt-controller` 和 `#interrupt-cells = <2>`，consumer 通过 `interrupts-extended = <&stm32gpio 37 2>` 描述中断来源、硬件 pin 和触发模式。
- STM32 GPIO DM 适配层注册 `struct rt_device_pin`，调用 `pin_api_init()` 提供 GPIO provider 能力；当节点带 `interrupt-controller` 时，再调用 `pin_pic_init()` 注册 GPIO pin PIC。
- STM32 HAL GPIO 驱动侧补充 DM IRQ mode 配置入口，并在 EXTI ISR 中把触发的 pin 转交给 `pin_pic_handle_isr()`，再由 PIC 分发到 `rt_pic_attach_irq()` 注册的处理函数。

需要注意：当前通用 `components/drivers/pin/dev_pin_dm.c` 中 `pin_dm_ops` 的 `.irq_enable` 指向 `pin_dm_irq_mask()`，而该函数会调用底层 `pin_irq_enable(..., 0)`。因此只调用 `rt_pic_irq_enable()` 时，STM32 EXTI 可能没有真正打开。当前 PC5 按键主路径已经切换到通用 `gpio-keys`，该路径使用 legacy pin IRQ API，不依赖 OFW PIC 中断映射。

## 后续扩展建议

- 按设备类型继续添加 platform driver，例如 key、pwm、i2c 设备的 OFW consumer 验证。
- 如果要把串口也迁移到 DM，需要单独适配 STM32 UART 的 platform driver，并处理 console 初始化顺序。
