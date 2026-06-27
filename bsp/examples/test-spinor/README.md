# SPI NOR Flash 测试 (test-spinor)

## 一句话描述

SPI NOR Flash 综合测试，包含 SFUD 通用驱动、FAL 抽象层、掉电保护和 XIP RAM 代码测试。

## 调用关系

```
MSH Shell → test-spinor → SFUD / FAL → AIC SPINOR 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_sfud.c` | SFUD（Serial Flash Universal Driver）测试 |
| `test_fal.c` | FAL（Flash Abstraction Layer）操作与性能基准测试 |
| `fal_power_loss_protection.c` | FAL 掉电保护机制验证 |
| `test_xip_ramcode.c` | XIP 模式下 RAM 代码执行测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `RT_USING_FAL` | 启用 FAL 抽象层 | n | - |
| `RT_USING_SFUD` | 启用 SFUD 通用 Flash 驱动 | n | RT_USING_SPI |
| `AIC_XIP_FLASH_WRITE` | 启用 XIP Flash 写入 | n | - |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options (multiple locations)
    └─ FAL / SFUD / XIP (multiple configs)
```
## 使用说明

### `sf` — SPI Flash (SFUD) 操作

**参数说明：**

| 参数 | 说明 |
|------|------|
| `sf probe [spi_device]` | 探测并初始化 SPI Flash |
| `sf read <addr> <size> [dis_print]` | 读取数据（1: 不打印 / 0: 打印） |
| `sf write <addr> <data1> ... <dataN>` | 写入字节数据 |
| `sf erase <addr> <size>` | 擦除指定范围 |
| `sf status [<volatile> <status>]` | 读写状态寄存器 |
| `sf bench` | 全芯片性能基准测试（⚠️ 会擦除全芯片） |
| `sf write_len <addr> <size>` | 写入指定长度数据 |
| `sf read_erase_write <addr> <size>` | 测试读-擦-写全流程速度 |

**使用示例：**

```bash
# 探测 spi10 上的 Flash
sf probe spi10

# 读取前 256 字节
sf read 0x0 0x100

# 写入两个字节
sf write 0x0 0xAA 0xBB

# 擦除 4KB
sf erase 0x0 0x1000

# 运行全芯片基准测试
sf bench

```

### `fal` — FAL 抽象层操作

**参数说明：**

| 参数 | 说明 |
|------|------|
| `fal probe [dev_name|part_name]` | 探测 Flash 设备或分区 |
| `fal read <addr> <size>` | 读取数据 |
| `fal write <addr> <data1> ... <dataN>` | 写入数据 |
| `fal erase <addr> <size>` | 擦除数据 |
| `fal bench <blk_size>` | 性能基准测试 |

**使用示例：**

```bash
# 探测 data 分区
fal probe data

# 读取前 256 字节
fal read 0x0 0x100

# 以 4KB 块大小进行基准测试
fal bench 4096

```

### `fal_pwr_loss` — FAL 掉电保护演示

**使用示例：**

```bash
# 运行掉电保护验证
fal_pwr_loss

```

### `xip_ramcode` — XIP 模式 RAM 代码执行

**使用示例：**

```bash
# 在 XIP 模式下执行 RAM 代码写入 Flash
xip_ramcode

```

## 依赖说明

- **上游依赖**：SFUD 库, FAL 组件, AIC SPINOR 驱动
- **下游被依赖**：无
