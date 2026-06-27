# MMC 存储测试 (test-mmc)

## 一句话描述

MMC（MultiMediaCard/eMMC/SD）存储设备综合测试，支持读写、擦除、dump 等操作。

## 调用关系

```
MSH Shell → test-mmc → AIC SDMC 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_mmc.c` | MMC 设备读写擦测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_MMC_TEST` | 启用 MMC 测试命令 | n | AIC_SDMC_DRV && !DRIVER_BARE_DRV_EN |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable MMC test command
```

## 使用说明

### `mmc` — MMC 设备操作命令

**参数说明：**

| 参数 | 说明 |
|------|------|
| `mmc dump <dev> <blk_offset> <blk_cnt>` | dump 指定块范围数据 |
| `mmc read <dev> <blk_offset> <blk_cnt> <addr>` | 读取数据到内存地址 |
| `mmc write <dev> <blk_offset> <blk_cnt> <addr>` | 从内存地址写入数据 |
| `mmc erase <dev> <blk_offset> <blk_cnt>` | 擦除指定块范围 |

**使用示例：**

```bash
# dump mmc0 第 0 块 1 个块的数据
mmc dump mmc0 0 1

# 读 4 块到 0x42000000
mmc read mmc0 0 4 0x42000000

# 从 0x42000000 写入 4 块
mmc write mmc0 0 4 0x42000000

# 擦除 mmc0 前 4 块
mmc erase mmc0 0 4

```

## 依赖说明

- **上游依赖**：AIC SDMC 驱动
- **下游被依赖**：无
