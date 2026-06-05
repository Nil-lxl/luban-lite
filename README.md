# Luban-Lite SDK

Luban-Lite 是匠芯创(ArtInChip)科技有限公司的嵌入式 RTOS SDK，旨在为 ArtInChip 系列芯片提供完整的软件开发环境。

## 项目简介

Luban-Lite 设计目标是：

- **多内核支持**：兼容 RT-Thread、FreeRTOS、uCOS-II，支持 baremetal 裸机模式
- **多芯片覆盖**：支持 d11x、d12x、d13x、d21x、g72x、g73x 等 SoC 系列
- **完整软件栈**：提供驱动框架、中间件、文件系统、网络协议栈等完整生态

## 架构概览

### 四级抽象模型

Luban-Lite 采用四级抽象模型，实现跨平台复用：

```
┌────────────────────────────────────────────────────────────┐
│  Level 1: SoC                                              │
│  bsp/artinchip/sys/{chip}/                                 │
│  职责：芯片级初始化、中断向量、时钟树、启动代码              │
├────────────────────────────────────────────────────────────┤
│  Level 2: Board                                            │
│  target/{chip}/{board}/                                    │
│  职责：板级差异配置、pinmux、分区表                         │
├────────────────────────────────────────────────────────────┤
│  Level 3: Kernel                                           │
│  kernel/{rt-thread,freertos,ucos-ii,baremetal}/            │
│  职责：操作系统内核                                        │
├────────────────────────────────────────────────────────────┤
│  Level 4: Application                                      │
│  application/{os}/                                         │
│  职责：用户应用代码                                        │
└────────────────────────────────────────────────────────────┘
```

### 驱动三层架构

驱动框架分为三层，实现 OS 无关的硬件抽象：

```
┌─────────────────────────────────┐
│ RT-Thread Driver Framework      │  ← RT-Thread 提供的驱动模型
├─────────────────────────────────┤
│ AIC Driver Layer (drv/)         │  ← 对接 RT-Thread，使用 OSAL 接口
│  - 负责中断注册、互斥锁、信号量  │
│  - 通过 rt_device 注册设备       │
├─────────────────────────────────┤
│ AIC HAL Layer (hal/)            │  ← 纯硬件操作，不依赖任何 OS
│  - 寄存器级别的功能接口          │
│  - 可被 baremetal 应用直接调用   │
└─────────────────────────────────┘
```

**HAL 层原则**：
- 不使用互斥锁、信号量
- 不注册中断
- 不依赖任何 OS 接口

**DRV 层原则**：
- 使用 OSAL 接口，不直接调用具体内核 API
- 中断注册、互斥锁、信号量操作放在 DRV 层

### OSAL 抽象层

OSAL (OS Abstraction Layer) 提供统一的 OS 接口，实现跨内核复用：

```
kernel/common/include/osal/aic_osal.h
├── aic_osal_rtthread.h   ← RT-Thread 适配
├── aic_osal_freertos.h   ← FreeRTOS 适配
├── aic_osal_ucos_ii.h    ← uCOS-II 适配
└── aic_osal_baremetal.h  ← 裸机适配
```

## 构建简介

Luban-Lite 采用 SCons + Kconfig 构建系统，并提供 **OneStep** 增强命令行工具，大幅提升开发效率。

### SCons 基础命令

| 命令 | 说明 |
|------|------|
| `scons --list-def` | 列出所有支持的板卡配置 |
| `scons --apply-def=<config>` | 应用指定板卡配置（可用编号或配置名） |
| `scons --menuconfig` | 打开配置菜单，修改 SDK 配置 |
| `scons` | 编译当前项目 |
| `scons -c` | 清理编译产物 |
| `scons --info` | 查看当前项目配置信息 |
| `scons --save-def` | 保存当前配置为 defconfig |
| `scons --distclean` | 清除工具链和输出目录 |
| `scons --verbose` | 编译时打印详细信息 |

**典型编译流程**：

```bash
# 1. 查看支持的板卡配置
scons --list-def

# 2. 选择板卡配置（使用编号或配置名）
scons --apply-def=3
# 或
scons --apply-def=d12x_demo68-nor_rt-thread_helloworld_defconfig

# 3. 配置参数（可选）
scons --menuconfig

# 4. 编译
scons
```

### OneStep 增强命令（推荐）

OneStep 是 ArtInChip 对 SCons 的二次封装，提供更高效的快捷命令，实现**任意目录、一步即达**。

**环境设置**：
- **Windows**：自动集成到 `win_cmd.bat` 和 `win_env.bat`，启动后即可使用
- **Linux**：执行 `source tools/onestep.sh` 即可使用

**常用命令**：

| 命令 | 说明 | 示例 |
|------|------|------|
| `lunch [keyword]` | 选择并加载指定配置方案 | `lunch d13x` 或 `lunch mmc` |
| `m` 或 `mb` | 编译 bootloader 和应用，生成最终镜像 | `m` |
| `ma` | 仅编译应用 | `ma` |
| `mu` 或 `ms` | 仅编译 bootloader | `mu` |
| `c` | 清理 bootloader 和应用 | `c` |
| `mc` | 清理并重新编译所有 | `mc` |
| `menuconfig` 或 `me` | 配置应用（RT-Thread） | `me` |
| `bm` | 配置 bootloader | `bm` |
| `i` | 查看当前项目信息 | `i` |
| `list` | 列出所有配置方案 | `list` |
| `croot` 或 `cr` | 跳转到 SDK 根目录 | `cr` |
| `cout` 或 `co` | 跳转到编译输出目录 | `co` |
| `ctarget` 或 `ct` | 跳转到目标板卡目录 | `ct` |
| `godir [keyword]` | 跳转到指定目录 | `gd gpio` |
| `aicupg` | 烧录镜像到目标板 | `aicupg` |

**OneStep 典型工作流**：

```bash
# 1. 选择配置方案（支持关键字搜索）
lunch d13x_demo88

# 2. 编译并生成镜像（一步完成）
m

# 3. 烧录到目标板
aicupg
```

**查看当前项目信息**：

```bash
i
# 输出示例：
# Target app: application/rt-thread/helloworld
# Target chip: d12x
# Target arch: riscv32
# Target board: target/d12x/demo68-nor
# Target kernel: kernel/rt-thread
# Defconfig file: target/configs/d12x_demo68-nor_rt-thread_helloworld_defconfig
```

### 方案配置

项目由 defconfig 文件定义，位于 `target/{chip}/{board}/` 目录：

```
target/d12x/demo68-nand/    →  d12x 芯片, demo68-nand 板卡
target/d13x/demo88-nor/     →  d13x 芯片, demo88-nor 板卡
target/d21x/demo100-nor/    →  d21x 芯片, demo100-nor 板卡
```

### 编译输出

编译成功后，在 `output/` 目录生成以下结构：

```
output/d12x_demo68-nor_rt-thread_helloworld/
├── application/    # 应用层 .o 文件
├── bsp/            # BSP 层 .o 文件（驱动、外设）
├── images/         # 镜像文件、符号表
│   ├── bootloader.aic
│   ├── os.aic
│   └── d12x_demo68-nor_v1.0.0.img
├── kernel/         # 内核 .o 文件
├── libs/           # 库文件
├── packages/       # 组件包 .o 文件
└── target/         # 板级配置 .o 文件（board.o, pinmux.o, sys_clk.o）
```

### 配置项

Kconfig 分为两类：
- **Kconfig.dev**：设备参数（波特率、引脚号等）
- **Kconfig.drv**：驱动参数（DMA 开关、DEBUG 开关等）

## 编码规范

本 SDK 沿用 RT-Thread 编码规范，详见：
https://www.rt-thread.org/document/site/#/rt-thread-version/rt-thread-standard/development-guide/coding-style/coding-style

### Git 提交规范

 提交的描述信息需要按照下面的格式进行

 - 全部使用英文
  - _标题_： 模块名字：小于 50 字符的简述
  - _内容_： 对所解决的问题的详细描述，最好包括背景，解决方法，以及其他有用的信息。
  - _脚注_： 额外的提醒，比如一些不兼容等，需要特别注意的事项；解决的bug的链接等。
  - _标题_ 与 _内容_ 之间应该要有一行空白
  - _内容_ 与 _脚注_ 之间应该要有一行空白

```
HEADER: <Module>: <Short description>
BLANK :
BODY  : <Detail description about this commit>
BLANK :
FOOTER: <Addtional information>
```

### SDK 补充约定

| 项目 | RT-Thread 规范 | SDK 补充 |
|------|---------------|----------|
| 基本类型 | 使用标准 `uint32_t` 等 | 使用 `aic_core.h` 中定义的 `u32`/`s32` 等 |
| 日志 | `rt_kprintf()` | 使用 `LOG_TAG` + `aic_log.h` 的 LOG 宏 |
| OS 接口 | 直接调用 RT-Thread API | **必须**使用 OSAL 接口 (`aicos_*`) |
| HAL 层 | — | 不使用互斥锁、不注册中断 |
| DRV 层 | — | 通过 `rt_device` 框架注册设备 |
| 命名 | — | HAL 函数: `hal_xxx_action()`, DRV 函数: `drv_xxx_action()` |

### 类型定义

```c
// 使用 SDK 类型（推荐）
u32 value = 0;
s32 result = -1;

// 而非标准类型
uint32_t value = 0;  // 不推荐
int32_t result = -1; // 不推荐
```

### 日志使用

```c
#define LOG_TAG     "uart"  // 每个模块开头定义
#include "aic_core.h"

LOG_D("debug message: %d", value);
LOG_I("info message");
LOG_W("warning message");
LOG_E("error message: %d", err);
```

### OSAL 使用

```c
// 正确 ✅
aicos_msleep(100);
aicos_mutex_take(&mutex, AICOS_WAIT_FOREVER);

// 错误 ❌
rt_thread_mdelay(100);     // 直接调用 RT-Thread API
vTaskDelay(100);           // 直接调用 FreeRTOS API
```

## 文档索引

| 文档 | 说明 |
|------|------|
| [AGENTS.md](AGENTS.md) | AI Agent 入口文档（构建命令、约定、流程） |
| [CODEMAP.md](CODEMAP.md) | 源码文件地图（精确路径索引） |
| [CONTRIBUTING.md](CONTRIBUTING.md) | 开发流程 Checklist |

## 在线文档

- **SDK 文档**：https://aicdoc.artinchip.com/topics/sdk/luban-lite-user-guide-lite.html

## 许可证

Apache License 2.0

## 版权

Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
