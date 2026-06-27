# MTD 测试 (test-mtd)

测试 MTD（内存技术设备）的读写和擦除功能。

## 调用关系

```
Bare Shell → test-mtd → AIC MTD 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `mtd.c` | 测试实现 |
| `mtd_example.c` | 使用示例 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_MTD_BARE_TEST` | 启用 MTD 裸机测试命令 | n | `AIC_MTD_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 MTD 裸机测试命令
```

## 使用说明

### `mtd` — MTD 读写操作

**子命令说明：**

| 子命令 | 说明 |
|--------|------|
| `list` | 列出所有 MTD 设备 |
| `bypass <status>` | SPIENC 旁路控制：1=旁路，0=启用 |
| `dump <part> <offset> <size>` | Dump 指定分区的数据 |
| `alloc <size>` | 分配指定大小的测试缓冲区 |
| `free` | 释放测试缓冲区 |
| `read <part> <addr> <offset> <size>` | 从 MTD 分区读取数据到内存 |
| `erase <part> <offset> <size>` | 擦除 MTD 分区指定区域 |
| `write <part> <addr> <offset> <size>` | 将内存数据写入 MTD 分区 |
| `oobdump <part> <offset>` | Dump OOB 数据（仅 NAND） |
| `oobread <part> <addr> <offset>` | 读取 OOB 数据（仅 NAND） |
| `oobwrite <part> <addr> <offset>` | 写入 OOB 数据（仅 NAND） |
| `contread <part> <addr> <offset> <size>` | 连续读取（仅 NAND，需 `AIC_SPINAND_CONT_READ`） |
| `isbad <part> <block>` | 检查坏块（仅 NAND） |

**使用示例：**

```bash
# 列出所有 MTD 设备
mtd list

# 分配 4096 字节缓冲区并填充测试数据
mtd alloc 4096
# 假设输出 Alloc buffer: 0x3e146e0
mtd_gendata l 0x3e146e0 0x100 0xA0a1a2a3

# 擦除并写入 NAND 分区
mtd erase nand0 0 0x1000
mtd write nand0 0x3e146e0 0 0x1000

# 读取并验证数据
mtd read nand0 0x3e146e0 0 0x1000
mtd dump nand0 0 0x100

# 释放缓冲区
mtd free
```

### `mtd_gendata` — 生成测试数据到内存

**参数说明：**

| 参数 | 说明 |
|------|------|
| `[mode]` | 数据宽度：`b` (字节,默认) / `w` (16 位) / `l` (32 位) |
| `<addr>` | 目标内存地址（十六进制） |
| `<cnt>` | 数据个数 |
| `[value]` | 填充值（省略则使用递增随机值） |

**使用示例：**

```bash
# 字节模式填充 64 个随机值
mtd_gendata 0x40000000 64

# 字节模式填充 64 个 0xAA
mtd_gendata 0x40000000 64 0xAA

# 16 位模式填充 64 个 0xA5
mtd_gendata w 0x40000000 64 0xA5

# 32 位模式填充 64 个 0x5A
mtd_gendata l 0x40000000 64 0x5A
```

### `mtd_usage` — MTD 使用示例

**使用示例：**

```bash
# 运行 MTD 读写和刷新示例
mtd_usage
```

## 依赖说明

- **上游依赖**：AIC MTD 驱动
- **下游被依赖**：无
