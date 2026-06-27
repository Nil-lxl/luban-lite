# SPI NAND Flash 测试 (test-spinand)

## 一句话描述

SPI NAND Flash 设备 MTD 接口测试，支持 ID 读取、读写、擦除、OOB 操作等。

## 调用关系

```
MSH Shell → test-spinand → RT-Thread MTD NAND → AIC SPINAND 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_mtd.c` | MTD NAND 设备测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `RT_USING_MTD_NAND` | 启用 MTD NAND 支持 | n | - |
| `RT_MTD_NAND_DEBUG` | 启用 MTD NAND 调试命令 | n | RT_USING_MTD_NAND |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ RT-Thread Components
    └─ Device Drivers
      └─ MTD
        └─ RT-Thread MTD NAND debug (via RT-Thread menuconfig)
```
## 使用说明

### `mtd_nand` — MTD NAND 设备操作

**参数说明：**

| 参数 | 说明 |
|------|------|
| `mtd_nand id <dev>` | 读取 NAND 设备 ID |
| `mtd_nand read <dev> <page> <len>` | 读取指定页数据 |
| `mtd_nand readcont <dev> <page> <len> <count>` | 连续读取多页 |
| `mtd_nand readoob <dev> <page> <len>` | 读取 OOB 数据 |
| `mtd_nand write <dev> <page> <len>` | 写入数据 |
| `mtd_nand erase <dev> <block>` | 擦除指定块 |
| `mtd_nand eraseall <dev>` | 擦除全部 |

**使用示例：**

```bash
# 读取 spinand0 的 ID
mtd_nand id spinand0

# 读取第 0 页 2048 字节
mtd_nand read spinand0 0 2048

# 擦除第 0 块
mtd_nand erase spinand0 0

```

## 依赖说明

- **上游依赖**：RT-Thread MTD NAND 框架, AIC SPINAND 驱动
- **下游被依赖**：无
