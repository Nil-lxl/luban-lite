# GPAI 模拟输入测试 (test-gpai)

## 一句话描述

GPAI（General Purpose Analog Input）通用模拟输入测试，支持通道采样、电压阈值和集成测试。

## 调用关系

```
MSH Shell → test-gpai → AIC GPAI 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_gpai.c` | GPAI 基础采样测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_GPAI_TEST` | 启用 GPAI 驱动测试命令 | n | AIC_GPAI_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable GPAI driver test command
```

## 使用说明

### `test_gpai` — GPAI 模拟输入采样

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-c, --channel <n>` | 选择通道号 [0, N-1]，默认 0 |
| `-t, --voltage <v>` | 设置默认电压阈值 |
| `-n, --number <n>` | 设置采样次数 |
| `-h, --help` | 显示帮助信息 |

**使用示例：**

```bash
# 通道 4 采样 100 次，阈值 3V
test_gpai -c 4 -n 100 -t 3

# 通道 0 默认采样
test_gpai -c 0

```

## 依赖说明

- **上游依赖**：AIC GPAI 驱动
- **下游被依赖**：无
