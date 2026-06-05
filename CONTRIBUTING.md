# Luban-Lite SDK 开发指南

本文档提供 Luban-Lite SDK 的开发流程和检查清单，帮助开发者正确添加新模块、板卡和测试示例。

## 开发流程概览

| 章节 | 内容 | 示例 |
|------|------|------|
| 新增 HAL 模块 | 纯硬件抽象层实现 | GPIO、UART、SPI 等 |
| 新增 DRV 模块 | 驱动层实现（对接 RT-Thread） | 显示驱动、存储驱动等 |
| 新增外设驱动 | 外部芯片驱动实现 | Camera、Touch、Codec 等 |
| 新增板卡配置 | 板级配置和 defconfig | demo68-nand、evb 等 |
| 新增测试示例 | 功能测试和示例程序 | test-gpio、test-spi 等 |
| 代码提交规范 | Git 提交和代码审查 | commit message 格式 |

## 一、新增 HAL 模块

HAL (Hardware Abstraction Layer) 层负责纯硬件操作，不依赖任何 OS。

### 1.1 创建目录结构

```
bsp/artinchip/hal/{module}/
├── aic_hal_{module}.c          # HAL 实现源码
└── SConscript                  # 构建脚本
```

### 1.2 添加头文件

在 `bsp/artinchip/include/hal/` 目录下创建头文件：

```c
// hal_{module}.h 或 aic_hal_{module}.h
#ifndef __HAL_{MODULE}_H__
#define __HAL_{MODULE}_H__

#include "aic_core.h"

// HAL 接口声明
s32 hal_{module}_init(void);
s32 hal_{module}_deinit(void);
// ...

#endif
```

### 1.3 实现源码

```c
#define LOG_TAG     "hal_{module}"
#include "aic_core.h"
#include "hal_{module}.h"

// 纯硬件操作，不使用互斥锁、不注册中断
s32 hal_{module}_init(void)
{
    // 寄存器操作
    // 时钟配置
    // ...
    return 0;
}
```

### 1.4 添加 SConscript

```python
from building import *

cwd = GetCurrentDir()
src = Glob('*.c')
CPPPATH = [cwd]

group = DefineGroup('hal_{module}', src, depend=['AIC_{MODULE}_HAL'], CPPPATH=CPPPATH)
Return('group')
```

### 1.5 HAL 模块检查清单

| 检查项 | 要求 | 示例 |
|--------|------|------|
| 目录结构 | `bsp/artinchip/hal/{module}/` | `hal/gpio/` |
| 头文件 | `bsp/artinchip/include/hal/` | `hal_gpio.h` |
| 命名规范 | `hal_{module}_{func}()` | `hal_gpio_set_value()` |
| 日志标签 | `#define LOG_TAG "hal_{module}"` | `LOG_TAG "hal_gpio"` |
| OS 依赖 | **禁止**使用任何 OS 接口 | 不使用互斥锁、信号量 |
| 中断注册 | **禁止**在 HAL 层注册中断 | 中断注册放在 DRV 层 |
| SConscript | 添加构建脚本 | `DefineGroup('hal_{module}', ...)` |

## 二、新增 DRV 模块

DRV 层对接 RT-Thread 驱动框架，使用 OSAL 接口。

### 2.1 创建目录结构

```
bsp/artinchip/drv/{module}/
├── aic_drv_{module}.c          # DRV 实现源码
├── Kconfig                     # 配置选项
└── SConscript                  # 构建脚本
```

### 2.2 添加头文件（如需要）

在 `bsp/artinchip/include/drv/` 目录下创建头文件：

```c
// aic_drv_{module}.h
#ifndef __DRV_{MODULE}_H__
#define __DRV{MODULE}_H__

#include "aic_core.h"

// DRV 接口声明
s32 drv_{module}_init(void);
// ...

#endif
```

### 2.3 实现源码

```c
#define LOG_TAG     "drv_{module}"
#include "aic_core.h"
#include "aic_drv_{module}.h"
#include "hal_{module}.h"

static rt_device_t g_{module}_dev;

// 使用 OSAL 接口，不直接调用 RT-Thread API
static rt_err_t _{module}_init(rt_device_t dev)
{
    // 调用 HAL 层接口
    hal_{module}_init();

    // 注册中断（使用 OSAL）
    // aicos_request_irq(...);

    return RT_EOK;
}

// RT-Thread 设备操作函数
static struct rt_device_ops _{module}_ops = {
    .init = _{module}_init,
    // ...
};

s32 drv_{module}_init(void)
{
    // 注册 RT-Thread 设备
    g_{module}_dev = rt_device_create(RT_Device_Class_Char, 0);
    g_{module}_dev->ops = &_{module}_ops;
    rt_device_register(g_{module}_dev, "{module}", RT_DEVICE_FLAG_RDWR);

    return 0;
}
```

### 2.4 添加 Kconfig

```
config AIC_{MODULE}_DRV
    bool "Enable {MODULE} driver"
    default n
    select AIC_{MODULE}_HAL
    help
      {MODULE} driver support.

if AIC_{MODULE}_DRV
config AIC_{MODULE}_DEBUG
    bool "Enable {MODULE} debug"
    default n
endif
```

### 2.5 添加 SConscript

```python
from building import *

cwd = GetCurrentDir()
src = Glob('*.c')
CPPPATH = [cwd]

group = DefineGroup('drv_{module}', src, depend=['AIC_{MODULE}_DRV'], CPPPATH=CPPPATH)
Return('group')
```

### 2.6 DRV 模块检查清单

| 检查项 | 要求 | 示例 |
|--------|------|------|
| 目录结构 | `bsp/artinchip/drv/{module}/` | `drv/gpio/` |
| 头文件 | `bsp/artinchip/include/drv/` | `aic_drv_gpio.h` |
| 命名规范 | `drv_{module}_{func}()` | `drv_gpio_init()` |
| 日志标签 | `#define LOG_TAG "drv_{module}"` | `LOG_TAG "drv_gpio"` |
| OSAL 使用 | **必须**使用 OSAL 接口 | `aicos_msleep()` 而非 `rt_thread_mdelay()` |
| 设备注册 | 通过 `rt_device` 注册 | `rt_device_register()` |
| 中断注册 | 在 DRV 层注册中断 | `aicos_request_irq()` |
| Kconfig | 添加配置选项 | `AIC_{MODULE}_DRV` |
| SConscript | 添加构建脚本 | `DefineGroup('drv_{module}', ...)` |

## 三、新增外设驱动

外设驱动位于 `bsp/peripheral/` 目录，用于支持外部芯片。

### 3.1 创建目录结构

```
bsp/peripheral/{type}/{chip}/
├── drv_{chip}.c                # 驱动实现
├── {chip}_regs.h               # 寄存器定义（如需要）
├── Kconfig                     # 配置选项
└── SConscript                  # 构建脚本
```

### 3.2 实现驱动

```c
#define LOG_TAG     "drv_{chip}"
#include "aic_core.h"
#include "drv_{type}.h"          // 统一接口头文件

// 对接统一接口
static s32 _{chip}_init(struct drv_{type}_dev *dev)
{
    // 初始化外部芯片
    // ...
    return 0;
}

struct drv_{type}_ops g_{chip}_ops = {
    .init = _{chip}_init,
    // ...
};
```

### 3.3 添加 Kconfig

```
config DRV_{CHIP}
    bool "Enable {CHIP} {TYPE} driver"
    default n
    select AIC_{TYPE}_DRV
    help
      {CHIP} {TYPE} driver support.
```

### 3.4 外设驱动检查清单

| 检查项 | 要求 | 示例 |
|--------|------|------|
| 目录结构 | `bsp/peripheral/{type}/{chip}/` | `peripheral/camera/gc0308/` |
| 命名规范 | `drv_{chip}.c` | `drv_gc0308.c` |
| 统一接口 | 对接 `{type}` 统一接口 | `drv_camera.h` |
| 寄存器定义 | `{chip}_regs.h` | `gc0308_regs.h` |
| Kconfig | 添加配置选项 | `DRV_GC0308` |
| SConscript | 添加构建脚本 | `DefineGroup('drv_{chip}', ...)` |

## 四、新增板卡配置

板卡配置位于 `target/{chip}/{board}/` 目录。

### 4.1 创建目录结构

```
target/{chip}/{board}/
├── defconfig                   # 默认配置
├── board.c                     # 板级初始化
├── pinmux.c                    # 引脚复用配置
└── Kconfig                     # 板卡配置选项（如需要）
```

### 4.2 创建 defconfig

```
# Project configuration
CONFIG_PRJ_CHIP="{chip}"
CONFIG_PRJ_BOARD="{board}"
CONFIG_PRJ_KERNEL="rt-thread"
CONFIG_PRJ_APP="baremetal/helloworld"

# Driver configuration
CONFIG_AIC_GPIO_DRV=y
CONFIG_AIC_UART_DRV=y
# ...
```

### 4.3 实现板级初始化

```c
// board.c
#define LOG_TAG     "board"
#include "aic_core.h"

s32 board_init(void)
{
    // 板级初始化
    // 时钟配置
    // 引脚复用
    // ...
    return 0;
}
```

### 4.4 板卡配置检查清单

| 检查项 | 要求 | 示例 |
|--------|------|------|
| 目录结构 | `target/{chip}/{board}/` | `target/d12x/demo68-nand/` |
| defconfig | 必须包含项目配置 | `CONFIG_PRJ_CHIP`, `CONFIG_PRJ_BOARD` |
| board.c | 板级初始化函数 | `board_init()` |
| pinmux.c | 引脚复用配置 | GPIO、外设引脚配置 |
| 构建验证 | `scons --menuconfig` 可选择 | 在配置菜单中可见 |

## 五、新增测试示例

测试示例位于 `bsp/examples/` 目录，以 Shell 命令形式提供。

### 5.1 创建目录结构

```
bsp/examples/test-{module}/
├── test_{module}.c             # 测试实现
├── Kconfig                     # 配置选项
└── SConscript                  # 构建脚本
```

### 5.2 实现测试命令

```c
#define LOG_TAG     "test_{module}"
#include "aic_core.h"

static void _test_{module}_usage(void)
{
    rt_kprintf("Usage: test_{module} [option]\n");
    rt_kprintf("  test_{module} read    - Read test\n");
    rt_kprintf("  test_{module} write   - Write test\n");
}

static int _test_{module}(int argc, char **argv)
{
    if (argc < 2) {
        _test{module}_usage();
        return -1;
    }

    if (!strcmp(argv[1], "read")) {
        // 读测试
    } else if (!strcmp(argv[1], "write")) {
        // 写测试
    } else {
        _test{module}_usage();
    }

    return 0;
}

MSH_CMD_EXPORT_ALIAS(_test{module}, test_{module}, {MODULE} test);
```

### 5.3 添加 Kconfig

```
config AIC_{MODULE}_DEBUG
    bool "Enable {MODULE} debug/test"
    default n
    select AIC_{MODULE}_DRV
    help
      {MODULE} debug and test commands.
```

### 5.4 测试示例检查清单

| 检查项 | 要求 | 示例 |
|--------|------|------|
| 目录结构 | `bsp/examples/test-{module}/` | `examples/test-gpio/` |
| 命名规范 | `test_{module}.c` | `test_gpio.c` |
| Shell 命令 | 使用 `MSH_CMD_EXPORT_ALIAS` | `test_gpio` 命令 |
| 使用说明 | 提供 usage 函数 | `_test_gpio_usage()` |
| Kconfig | 添加 `AIC_{MODULE}_DEBUG` | `AIC_GPIO_DEBUG` |
| SConscript | 添加构建脚本 | `DefineGroup('test_{module}', ...)` |

## 六、代码提交规范

### 6.1 Commit Message 格式

```
<type>(<scope>): <subject>

<body>

<footer>
```

**type 类型**：
- `feat`: 新功能
- `fix`: 修复 bug
- `docs`: 文档更新
- `style`: 代码格式调整（不影响功能）
- `refactor`: 重构
- `test`: 测试相关
- `chore`: 构建或辅助工具变动

**示例**：
```
feat(drv): add PWM driver support

Add PWM driver for d12x/d13x series:
- HAL layer: hal_pwm.c
- DRV layer: drv_pwm.c
- Kconfig: AIC_PWM_DRV option

Signed-off-by: Zhang San <zhangsan@artinchip.com>
```

### 6.2 代码审查检查清单

| 检查项 | 要求 | 示例 |
|--------|------|------|
| 编码规范 | 遵循 RT-Thread 规范 + SDK 补充 | 见 README.md 编码规范章节 |
| 日志标签 | 每个 .c 文件定义 `LOG_TAG` | `#define LOG_TAG "module"` |
| OSAL 使用 | DRV 层使用 OSAL 接口 | 不直接调用 RT-Thread API |
| HAL 纯净 | HAL 层不依赖 OS | 不使用互斥锁、中断注册 |
| 头文件依赖 | 最小化头文件包含 | 只包含必要的头文件 |
| 注释规范 | 函数头注释 + 关键代码注释 | Doxygen 格式 |
| 测试验证 | 提交前本地测试通过 | `scons && scons run` |

### 6.3 提交前检查流程

```bash
# 1. 代码格式化
clang-format -i <modified_files>

# 2. 编译验证
scons -c && scons

# 3. 运行测试（如有）
scons run

# 4. 检查代码差异
git diff

# 5. 提交代码
git add <files>
git commit -s
git push
```

## 七、常见问题

### Q1: HAL 和 DRV 的区别？

| 层次 | 职责 | 依赖 | 示例 |
|------|------|------|------|
| HAL | 纯硬件操作 | 无 OS 依赖 | 寄存器读写、时钟配置 |
| DRV | 驱动框架对接 | 依赖 OSAL | 设备注册、中断处理 |

### Q2: 何时使用 OSAL？

**必须使用 OSAL 的场景**：
- DRV 层代码
- 需要跨内核复用的代码
- 应用层代码

**不应使用 OSAL 的场景**：
- HAL 层代码（纯硬件操作）
- 启动代码（OS 未初始化）

### Q3: 如何调试驱动？

1. 使能调试选项：`scons --menuconfig` → 选择 `AIC_{MODULE}_DEBUG`
2. 添加日志输出：`LOG_D()`, `LOG_I()`, `LOG_W()`, `LOG_E()`
3. 使用测试命令：`test_{module}`

### Q4: 如何添加新的 SoC 支持？

1. 创建 SoC 目录：`bsp/artinchip/sys/{chip}/`
2. 添加启动代码、中断向量、时钟配置
3. 创建 defconfig 模板：`target/{chip}/`
4. 更新构建脚本：`bsp/artinchip/SConscript`

## 八、参考文档

- [AGENTS.md](AGENTS.md) — AI Agent 上下文（构建命令、目录结构）
- [README.md](README.md) — 架构概览与编码规范
- [CODEMAP.md](CODEMAP.md) — 源码文件地图
- [RT-Thread 编码规范](https://www.rt-thread.org/document/site/#/development-guide/coding-style/coding-style)
- [SDK 在线文档](https://aicdoc.artinchip.com/topics/sdk/luban-lite-user-guide-lite.html)
