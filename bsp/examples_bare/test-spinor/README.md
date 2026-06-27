# SPI NOR 测试 (test-spinor)

测试 SPI NOR Flash 的读写和擦除功能。

## 调用关系

```
Bare Shell → test-spinor → AIC SPINOR 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `spinor.c` | 测试实现 |
| `spinor_benchmark.c` | 性能基准测试 |
| `spinor_example.c` | 使用示例 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_SPINOR_DRV_TEST` | 启用 SPI NOR 测试命令 | n | `AIC_SPINOR_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 SPI NOR 测试命令
```

## 使用说明

### `spinor` — SPI NOR 读写操作

**子命令说明：**

| 子命令 | 说明 |
|--------|------|
| `init <spi_bus_id>` | 初始化 SPI NOR Flash，指定 SPI 总线 ID |
| `dump <offset> <size>` | Dump 指定偏移地址的数据 |
| `read <addr> <offset> <size>` | 从 Flash 读取数据到 RAM 地址 |
| `erase <offset> <size>` | 擦除指定偏移地址的区域 |
| `write <addr> <offset> <size>` | 将 RAM 数据写入 Flash |
| `regs` | Dump 所有寄存器 |
| `regwrite <reg> <val>` | 写入指定寄存器 |
| `statuswrite <reg> <val> <volatile>` | 写入状态寄存器，`non-volatile` 表示非易失 |
| `regread <reg>` | 读取指定寄存器 |
| `uidread` | 读取芯片 UID |
| `se read <reg>` | 读取安全寄存器（需 `SFUD_USING_SECURITY_REGISTER`） |
| `se erase <reg>` | 擦除安全寄存器 |
| `se write <reg> <addr> <size>` | 写入安全寄存器 |
| `se lock <reg>` | 锁定安全寄存器 |

**使用示例：**

```bash
# 初始化 SPI NOR，总线 0
spinor init 0

# 读取 Flash 偏移 0 起的 256 字节到 RAM 0x40000000
spinor read 0x40000000 0 256

# 擦除并写入 Flash
spinor erase 0 0x1000
mtd_gendata l 0x40000000 64 0x5A
spinor write 0x40000000 0 256

# 写入状态寄存器（非易失）
spinor statuswrite 0x05 0x60 non-volatile

# Dump 数据
spinor dump 0 0x100
```

### `spinor_usage` — SPI NOR API 使用示例

**使用示例：**

```bash
# 运行 SPI NOR 读写和刷新示例
spinor_usage
```

### `spinor_benchmark` — SPI NOR 性能基准测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<spi_bus_id>` | SPI 总线 ID |

> **注意：** 此命令会擦除整个 SPI NOR Flash 上的所有数据。

**使用示例：**

```bash
# 在 SPI 总线 0 上运行基准测试（会擦除全部数据）
spinor_benchmark 0
```

## 依赖说明

- **上游依赖**：AIC SPINOR 驱动
- **下游被依赖**：无
