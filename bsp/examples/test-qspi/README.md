# QSPI/SPI 综合测试 (test-qspi)

## 一句话描述

QSPI 和 SPI 设备综合测试，支持主机/从机模式、异步传输、Bit 模式、Flash 块设备等多种场景。

## 调用关系

```
MSH Shell → test-qspi → AIC QSPI/SPI 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_qspidev.c` | QSPI 设备交互测试 |
| `test_spi.c` | SPI 设备基础测试 |
| `test_spi_async.c` | SPI 异步传输测试 |
| `test_spibit.c` | SPI Bit 模式测试 |
| `qspi_dev_example.c` | QSPI 设备配置示例 |
| `spi_dev_example.c` | SPI 设备配置示例 |
| `test_spiflash_dev_example.c` | SPI Flash 块设备操作示例 |
| `test_spislave_init.c` | QSPI 从机初始化 |
| `test_spislave_receiver.c` | QSPI 从机接收器 |
| `test_spislave_test.c` | QSPI 从机测试 |
| `test_spislave.h` | QSPI 从机头文件 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_QSPI_DRV_TEST` | 启用 SPI 驱动测试命令 | n | AIC_QSPI_DRV && !DRIVER_BARE_DRV_EN |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable SPI driver test command
```

## 使用说明

### `qspidev` — QSPI 设备交互命令

**参数说明：**

| 参数 | 说明 |
|------|------|
| `qspidev attach <bus> <dev>` | 将设备挂载到 QSPI 总线 |
| `qspidev init <name> <mode> <freq>` | 初始化 QSPI 设备 |
| `qspidev send <lines> <cmd> <addr> <dummy> <data_addr> <data_len>` | 发送数据 |
| `qspidev recv <lines> <cmd> <addr> <dummy> <data_addr> <data_len>` | 接收数据 |
| `qspidev sendhex <lines> <cmd> [addr] [dummy] <hex_data>` | 以 hex 格式发送 |
| `qspidev recvhex <lines> <cmd> [addr] [dummy] <data_len>` | 以 hex 格式接收并打印 |
| `qspidev sendlen <lines> <data_len>` | 发送指定长度数据 |
| `qspidev alloc <size>` | 分配测试缓冲区 |
| `qspidev free <addr>` | 释放测试缓冲区 |

**使用示例：**

```bash
# 挂载设备到 qspi1
qspidev attach qspi1 qtestdev

# 初始化：模式 3，50MHz
qspidev init qtestdev 3 50000000

# 读取 JEDEC ID（3 字节）
qspidev recvhex 111 9f - 8 3

# 发送命令 0xFF
qspidev sendhex 111 ff

# 读取 2KB 数据
qspidev recvhex 111 03 000000 0 0x800

```

### `test_spi` — SPI 基础测试

**使用示例：**

```bash
# 执行 SPI 基础收发测试
test_spi

```

### `spi_async` — SPI 异步传输测试

**使用示例：**

```bash
# 执行 SPI 异步传输
spi_async

```

### `spi_async_thread` — SPI 异步传输（线程模式）

**使用示例：**

```bash
# 线程模式 SPI 异步传输
spi_async_thread

```

### `test_spibit` — SPI Bit 模式测试

**使用示例：**

```bash
# 执行 SPI Bit 模式测试
test_spibit

```

### `flash_device_usage` — SPI Flash 块设备示例

**使用示例：**

```bash
# 查看 Flash 块设备操作示例
flash_device_usage

```

### `qspi_dev_usage` — QSPI 设备配置示例

**使用示例：**

```bash
# 查看 QSPI 设备配置示例
qspi_dev_usage

```

### `spi_dev_usage` — SPI 设备配置示例

**使用示例：**

```bash
# 查看 SPI 设备配置示例
spi_dev_usage

```

### `fakemem` — QSPI 从机测试

**使用示例：**

```bash
# 执行 QSPI 从机 fake memory 测试
fakemem

```

### `qpinmux` — QSPI 从机 pinmux 测试

**使用示例：**

```bash
# 测试 QSPI 从机引脚复用
qpinmux

```

## 依赖说明

- **上游依赖**：AIC QSPI 驱动, RT-Thread SPI 框架
- **下游被依赖**：无
