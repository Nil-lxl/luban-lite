# PBUS 并行总线测试 (test-pbus)

## 一句话描述

PBUS（Parallel Bus）并行总线测试，支持对外设地址空间进行读写操作。

## 调用关系

```
MSH Shell → test-pbus → AIC PBUS 硬件
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_pbus.c` | PBUS 读写测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_PBUS_DRV_TEST` | 启用 PBUS 驱动测试命令 | n | AIC_PBUS_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
    └─ Drivers examples
      └─ Enable PBUS driver test command
```
## 使用说明

### `test_pbus` — PBUS 读写操作

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<op>` | 操作类型：read 或 write |
| `<offset>` | 偏移地址（十六进制） |
| `<len/data>` | read 时为读取长度，write 时为写入数据 |

**使用示例：**

```bash
# 从偏移 0x100 读取 16 字节
test_pbus read 0x100 16

# 向偏移 0x100 写入 0xFF
test_pbus write 0x100 0xFF

```

## 依赖说明

- **上游依赖**：AIC PBUS 硬件
- **下游被依赖**：无
