# 分区表操作 (test-part)

## 一句话描述

分区表管理命令，支持查看分区列表和 dump GPT 分区表信息。

## 调用关系

```
MSH Shell → test-part → AIC 分区表模块
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `part.c` | 分区表操作命令 |
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

### `part` — 分区表操作命令

**参数说明：**

| 参数 | 说明 |
|------|------|
| `list <device>` | 列出指定设备的分区信息 |
| `dump <device>` | dump GPT 分区表原始数据 |

**使用示例：**

```bash
# 列出 mmc0 的分区列表
part list mmc0

# dump mmc0 的 GPT 分区表
part dump mmc0

```

## 依赖说明

- **上游依赖**：AIC 分区表模块, 存储设备驱动
- **下游被依赖**：无
