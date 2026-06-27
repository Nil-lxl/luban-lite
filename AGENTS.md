# Luban-Lite SDK — AI Agent Context

> 本文件为 AI 辅助编程工具提供项目上下文，请优先阅读。

## 一句话描述

Luban-Lite 是匠芯创(ArtInChip)的 RTOS SDK：
 - 支持RT-Thread/FreeRTOS/uCOS-II三种RTOS模式，默认使用RT-Thread
 - 支持Baremetal裸机模式
 - 覆盖 d11x/d12x/d13x/d21x/g72x/g73x 等 SoC
 - 采用 SCons+Kconfig 构建

## 构建命令

| 命令 | 说明 |
|------|------|
| `scons --menuconfig` | 打开配置菜单，选择芯片、板卡、应用 |
| `scons --list-def` | 列出当前支持的所有方案配置 |
| `scons --apply-def xx_defconfig/No.` | 加载指定方案的 defconfig 文件，应用到当前SDK配置 |
| `scons` | 编译当前项目 |
| `scons -c` | 清理编译产物 |
| `scons --info` | 查看当前项目配置信息 |

**方案的关键配置**：
- `CONFIG_PRJ_CHIP="d12x"` — 芯片型号
- `CONFIG_PRJ_BOARD="demo68-nand"` — 板卡名称
- `CONFIG_PRJ_KERNEL="rt-thread"` — 内核类型
- `CONFIG_PRJ_APP="baremetal/helloworld"` — 应用名称

## 烧录与调试

**输出镜像**：`output/{chip}_{board}_{kernel}_{app}/images/*.img`

**串口调试**：115200 波特率，8N1

**查看当前方案信息**：`scons --info`

## 当前配置速查

| 信息 | 来源 |
|------|------|
| 当前 defconfig | `target/configs/{chip}_{board}_{kernel}_{app}_defconfig` |
| 当前 Kconfig 配置 | 项目根目录 `.config` |
| 自动生成配置头文件 | 项目根目录 `cconfig.h`（编译时自动生成） |
| 板级配置 | `target/{chip}/{board}/`（pinmux、时钟树、分区表） |

## 目录结构速查

| 目录 | 职责 | 关键文件 |
|------|------|----------|
| `bsp/artinchip/` | AIC 驱动层 (drv + hal) | `drv/`, `hal/`, `include/` |
| `bsp/peripheral/` | 外设驱动 (camera/codec/touch...) | 各子目录 `drv_xxx.c` |
| `bsp/examples/` | 测试与示例程序 | `test-xxx/` |
| `kernel/` | OS 内核 | `rt-thread/`, `freertos/`, `baremetal/` |
| `kernel/common/` | OS 抽象层 (OSAL) | `include/osal/aic_osal.h` |
| `packages/artinchip/` | AIC 组件包 | `mpp/`, `lvgl-ui/`, `ota/`... |
| `packages/third-party/` | 第三方组件 | `lwip/`, `cherryusb/`... |
| `target/` | 板级配置 | `{chip}/{board}/defconfig` |
| `application/` | 应用层 | `rt-thread/`, `freertos/`, `baremetal/` |
| `tools/scripts/` | 构建脚本 | `aic_build.py` |

## 四级抽象模型

```
SoC (bsp/artinchip/sys/{chip}/)        ← 芯片级：中断、时钟、启动代码
  └─ Board (target/{chip}/{board}/)    ← 板级：pinmux、时钟、分区表
      └─ Kernel (kernel/{os}/)          ← 内核：RTOS 或 baremetal
          └─ App (application/{os}/)    ← 应用：用户代码入口
```

**SoC 列表**：d11x, d12p, d12x, d13x, d21x, g72x, g73x

**Kernel 列表**：rt-thread, freertos, ucos-ii, baremetal

## 驱动三层架构

```
┌─────────────────────────────────┐
│ RT-Thread Driver Framework      │ ← 由 RT-Thread 提供
├─────────────────────────────────┤
│ AIC Driver Layer (drv/)         │ ← 对接 RT-Thread，使用 OSAL 接口
├─────────────────────────────────┤
│ AIC HAL Layer (hal/)            │ ← 纯硬件操作，不依赖任何 OS
└─────────────────────────────────┘
```

**HAL 层原则**：
- 纯硬件操作，不依赖任何 OS
- 不使用互斥锁、信号量
- 不注册中断（中断注册在 DRV 层）
- 可被 baremetal 应用直接调用

**DRV 层原则**：
- 对接 RT-Thread 驱动框架
- 使用 OSAL 接口，不直接调用 RT-Thread API
- 负责中断注册、互斥锁、信号量
- 通过 `rt_device` 注册设备

**路径映射**：
- DRV 源码：`bsp/artinchip/drv/{module}/`
- DRV 头文件：`bsp/artinchip/include/drv/`
- HAL 源码：`bsp/artinchip/hal/{module}/`
- HAL 头文件：`bsp/artinchip/include/hal/`

## 关键类型与宏

| SDK 类型 | 标准类型 | 头文件 | 说明 |
|----------|----------|--------|------|
| `u32` | `uint32_t` | `aic_common.h` | 32位无符号整数 |
| `s32` | `int32_t` | `aic_common.h` | 32位有符号整数 |
| `u16` | `uint16_t` | `aic_common.h` | 16位无符号整数 |
| `u8` | `uint8_t` | `aic_common.h` | 8位无符号整数 |
| `ulong` | `unsigned long` | `aic_common.h` | 无符号长整数 |
| `bool` | `bool` | `aic_common.h` | 布尔类型 |

## OSAL 使用约定

驱动代码**必须**通过 OSAL 接口访问 OS 功能，禁止直接调用具体内核 API。

| 功能 | 正确用法 ✅ | 错误用法 ❌ |
|------|------------|------------|
| 毫秒延时 | `aicos_msleep(100)` | `rt_thread_mdelay(100)` / `vTaskDelay(100)` |
| 互斥锁 | `aicos_mutex_t` | `rt_mutex_t` / `SemaphoreHandle_t` |
| 信号量 | `aicos_sem_t` | `rt_sem_t` / `SemaphoreHandle_t` |
| 线程创建 | `aicos_thread_create()` | `rt_thread_create()` / `xTaskCreate()` |
| 内存分配 | `aicos_malloc()` | `rt_malloc()` / `pvPortMalloc()` |

**OSAL 头文件**：`kernel/common/include/osal/aic_osal.h`

## 日志规范

SDK 提供两套日志接口，详细定义参见 `kernel/rt-thread/components/utilities/ulog/ulog.h` 和 `bsp/common/include/aic_log.h`。

### 方式一：ulog 接口（推荐）

使用 `LOG_X()` 系列宏，需在文件开头定义 `LOG_TAG` 和 `LOG_LVL`：

```c
#define LOG_TAG     "module_name"
#define LOG_LVL     LOG_LVL_DBG
#include <ulog.h>

LOG_D("debug message");    /* 调试级别 */
LOG_I("info message");     /* 信息级别 */
LOG_W("warning message");  /* 警告级别 */
LOG_E("error message");    /* 错误级别 */
```

> **注意**：ulog 宏名是 `LOG_D` / `LOG_I` / `LOG_W` / `LOG_E`（不是 `LOG_DBG` 等）。

### 方式二：aic_log 接口（HAL 层 / 简单场景）

使用 `pr_*()` 系列函数，无需定义 LOG_TAG，头文件为 `bsp/common/include/aic_log.h`：

```c
#include "aic_log.h"

pr_debug("debug message");
pr_info("info message");
pr_warn("warning message");
pr_err("error message");
```

## 新增模块流程

### 新增 HAL 模块

1. 创建源码：`bsp/artinchip/hal/{module}/aic_hal_{module}.c`
2. 添加头文件：`bsp/artinchip/include/hal/hal_{module}.h` 或 `aic_hal_{module}.h`
3. 在 `bsp/artinchip/SConscript` 中添加编译条件（见下方模板）
4. 遵循 HAL 层原则：不使用互斥锁、不注册中断、不依赖 OS

> **注意**：HAL/DRV 模块**没有**独立的 SConscript 文件，全部由bsp层构建文件 `bsp/artinchip/SConscript` 通过 `Glob()` 收集。

**SConscript 模板**（添加到 `bsp/artinchip/SConscript`）：

```python
# {MODULE} driver
if GetDepend('AIC_{MODULE}_DRV'):
    if GetDepend('DRIVER_HAL_EN'):
        src += Glob('hal/{module}/*.c')
    if GetDepend('DRIVER_DRV_EN'):
        src += Glob('drv/{module}/*.c')
```

### 新增 DRV 模块

1. 创建源码：`bsp/artinchip/drv/{module}/aic_drv_{module}.c`
2. 添加头文件：`bsp/artinchip/include/drv/aic_drv_{module}.h`（如需要）
3. 添加 `Kconfig.drv`（驱动级配置项，如功能开关）
4. 添加 `Kconfig.dev`（设备级配置项，如引脚/通道选择）
5. 在 `bsp/artinchip/SConscript` 中添加编译条件（见上方模板）
6. 通过 `rt_device` 框架注册设备
7. 使用 OSAL 接口，不直接调用 RT-Thread API

> **Kconfig 约定**：`Kconfig.drv` 和 `Kconfig.dev` 由构建系统自动收集（放在 `drv/{module}/` 目录下即可），无需手动 source。主设备开关（如 `AIC_{MODULE}_DRV`）定义在公共 Kconfig 中。

### 新增外设驱动 (bsp/peripheral/)

1. 创建目录：`bsp/peripheral/{type}/{chip}/`
2. 实现源码：`drv_{chip}.c`
3. 添加寄存器定义：`{chip}_regs.h`
4. 对接统一接口（如 `drv_camera.h`）
5. 添加 Kconfig + SConscript

### 新增测试示例

1. 创建目录：`bsp/examples/test-{module}/`
2. 以 Shell 命令形式提供测试入口
3. 在 Kconfig 中添加 `AIC_{MODULE}_DEBUG` 开关

## 外设驱动速查

| 类型 | 目录 | 已支持芯片 |
|------|------|-----------|
| Camera | `bsp/peripheral/camera/` | nvp6158, gc0308, gc032a, ov2640, ov5640, sc035... |
| Touch | `bsp/peripheral/touch/` | gt9xx, ft6x36... |
| Codec | `bsp/peripheral/codec/` | es8388, ac107... |
| Wireless | `bsp/peripheral/wireless/` | hugeic... |
| RTC | `bsp/peripheral/rtc/` | pcf8563, rx8010... |

## AI 编码常见陷阱

- **HAL 层禁止 OS 依赖**：HAL 函数绝不能调用 `aicos_*` 接口，只能做纯硬件寄存器操作
- **外设驱动不强制 OSAL**：`bsp/peripheral/` 下的驱动可以直接使用 RT-Thread API（仅 AIC DRV 层强制使用 OSAL）
- **HAL 版本目录**：部分 HAL 模块有 `v1/` 和 `v2/` 子目录（如 `hal/dvp/`），API 不兼容，需根据 Kconfig 中的版本号配置选择

## 更多文档

- [CODEMAP.md](CODEMAP.md) — 源码文件地图（精确路径索引）
- [README.md](README.md) — 架构概览与编码规范
- [CONTRIBUTING.md](CONTRIBUTING.md) — 开发流程 Checklist

## 在线文档

- SDK 文档：https://aicdoc.artinchip.com/topics/sdk/luban-lite-user-guide-lite.html
