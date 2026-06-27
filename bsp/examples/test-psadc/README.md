# PSADC (PWM control Subsystem ADC) 测试 (test-psadc)

## 一句话描述

PSADC 测试，验证ADC采样功能。

## 调用关系

```
MSH Shell → test-psadc → AIC PSADC 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_psadc.c` | PSADC 电源电压采样测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_PSADC_TEST` | 启用 PSADC 驱动测试命令 | n | AIC_PSADC_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig -> Drivers options -> Drivers examples
```

## 使用说明

### `test_psadc` — PSADC 电源电压采样

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-v, --voltage <v>` | 设置参考电压 |
| `-n, --number <n>` | 采样次数 |
| `-w, --window <n>` | 窗口大小 |
| `-h, --help` | 显示帮助 |

**使用示例：**

```bash
# 默认参数采样
test_psadc

# 参考电压 3.3V，采样 100 次
test_psadc -v 3.3 -n 100

```

## 依赖说明

- **上游依赖**：AIC PSADC 驱动
- **下游被依赖**：无
