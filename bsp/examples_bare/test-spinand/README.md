# SPI NAND 测试 (test-spinand)

测试 SPI NAND Flash 的读写、擦除和坏块管理功能。

## 调用关系

```
Bare Shell → test-spinand → AIC SPINAND 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `spinand.c` | 测试实现 |
| `spinand_bbt_cmd.c` | 测试实现 |
| `spinand_bbt_example.c` | 使用示例 |
| `spinand_benchmark.c` | 性能基准测试 |
| `spinand_example.c` | 使用示例 |
| `spinand_nftl_example.c` | 使用示例 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_SPINAND_DRV_TEST` | 启用 SPI NAND 测试命令 | n | `AIC_SPINAND_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 SPI NAND 测试命令
```

## 使用说明

### `spinand` — SPI NAND 读写操作

**子命令说明：**

| 子命令 | 说明 |
|--------|------|
| `init <spi_bus_id>` | 初始化 SPI NAND，指定 SPI 总线 ID |
| `read <addr> <offset> <size>` | 从 NAND 读取数据到 RAM |
| `dump <offset> <size>` | Dump 指定偏移地址的数据 |
| `oobdump <offset>` | Dump 指定偏移地址的 OOB 数据 |
| `write <addr> <offset> <size>` | 将 RAM 数据写入 NAND |
| `erase <offset> <size>` | 擦除指定偏移地址的区域 |
| `bad <offset> <size>` | 检查指定区域的坏块 |
| `contread <offset> <size>` | 连续读取 |
| `regr <reg>` | 读取指定寄存器 |
| `eccen <enable>` | 启用/禁用 ECC（1=启用，0=禁用） |
| `bitflip <offset>` | 触发 bitflip 测试 |

**使用示例：**

```bash
# 初始化 SPI NAND，总线 0
spinand init 0

# 读取 0x20000 字节到 RAM 0x40000000
spinand read 0x40000000 0 0x20000

# Dump 偏移 0 起的 0x100 字节
spinand dump 0 0x100

# 擦除并写入 NAND
spinand erase 0 0x20000
spinand write 0x40000000 0 0x20000
```

### `spinand_usage` — SPI NAND API 使用示例

**使用示例：**

```bash
# 运行 SPI NAND 读写和刷新示例
spinand_usage
```

### `spinand_bbt` — SPI NAND 坏块表操作

> SPI NAND 会随机产生坏块，读写必须避开坏块。`spinand_bbt` 维护一张坏块表，自动将逻辑地址映射到物理地址。

**子命令说明：**

| 子命令 | 说明 |
|--------|------|
| `init <spi_bus> <offset> <size>` | 初始化坏块表，管理指定区域的坏块（默认保留 8 个块） |
| `dump <offset> <size>` | Dump 逻辑地址范围的数据 |
| `write <addr> <offset> <size>` | 将 RAM 数据写入逻辑地址 |
| `erase <offset> <size>` | 擦除逻辑地址范围 |
| `deinit` | 释放坏块表资源 |

**使用示例：**

```bash
# 初始化坏块表：总线 0，偏移 0x4800000，大小 0x800000
spinand_bbt init 0 0x4800000 0x800000

# 擦除逻辑地址范围
spinand_bbt erase 0x5000000 0x100000

# 写入数据
spinand_bbt write 0x40400000 0x5000800 0x80000

# Dump 数据
spinand_bbt dump 0x5000800 0x80000

# 释放资源
spinand_bbt deinit
```

### `spinand_bbt_benchmark` — SPI NAND 坏块表性能基准测试

**使用示例：**

```bash
# 运行坏块表基准测试
spinand_bbt_benchmark
```

### `spinand_benchmark` — SPI NAND 性能基准测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<spi_bus_id>` | SPI 总线 ID |

> **注意：** 此命令会擦除整个 SPI NAND Flash 上的所有数据。

**使用示例：**

```bash
# 在 SPI 总线 0 上运行基准测试（会擦除全部数据）
spinand_benchmark 0
```

### `nid` — 显示 NAND 制造商 ID

**使用示例：**

```bash
# 读取 NAND 制造商 ID（需先执行 spinand init）
nid
```

### `nftl_usage` — SPI NAND NFTL 使用示例

**使用示例：**

```bash
# 运行 NFTL 示例
nftl_usage
```

## 依赖说明

- **上游依赖**：AIC SPINAND 驱动
- **下游被依赖**：无
