# MMC/eMMC 测试 (test-mmc)

测试 MMC/eMMC 存储设备的读写功能。

## 调用关系

```
Bare Shell → test-mmc → AIC MMC 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_mmc.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_MMC_BARE_TEST` | 启用 MMC 裸机测试命令 | n | `AIC_MMC_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 MMC 裸机测试命令
```

## 使用说明

### `mmc` — MMC 读写操作

**子命令说明：**

| 子命令 | 说明 |
|--------|------|
| `help` | 显示帮助信息 |
| `dump <id> <blkoffset> <blkcnt>` | Dump 指定块的数据 |
| `erase <id> <blkoffset> <blkcnt>` | 擦除指定块区域 |
| `read <id> <blkoffset> <blkcnt> <addr>` | 从 MMC 读取数据到 RAM |
| `write <id> <blkoffset> <blkcnt> <addr>` | 将 RAM 数据写入 MMC |
| `scan <id> <blkoffset>` | 扫描 MMC 采样相位和延迟链 |
| `rpmb dump <id> <blkoffset> <blkcnt> [key_addr]` | Dump RPMB 数据 |
| `rpmb read <id> <blkoffset> <blkcnt> <addr> [key_addr]` | 读取 RPMB 到 RAM |
| `rpmb write <id> <blkoffset> <blkcnt> <addr> <key_addr>` | 将 RAM 写入 RPMB |
| `rpmb key <id> <key_addr>` | 烧录 RPMB 认证密钥（一次性操作） |
| `rpmb counter <id>` | 读取 RPMB 写入计数器 |

**使用示例：**

```bash
# Dump MMC 设备 0 的第 0 块
mmc dump 0 0 1

# 从 MMC 设备 0 读取 8 个块到 RAM 0x30000000
mmc read 0 0 8 0x30000000

# 将 RAM 数据写入 MMC 设备 0
mmc write 0 0 8 0x30000000

# 擦除 MMC 设备 0 的前 16 个块
mmc erase 0 0 16

# 读取 RPMB 写入计数器
mmc rpmb counter 0
```

## 依赖说明

- **上游依赖**：AIC MMC 驱动
- **下游被依赖**：无
