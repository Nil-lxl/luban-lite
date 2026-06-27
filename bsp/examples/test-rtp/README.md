# 电阻触摸屏测试 (test-rtp)

## 一句话描述

RTP（Resistive Touch Panel）电阻触摸屏测试，包含 ADC 采样和触摸绘图功能。

## 调用关系

```
MSH Shell → test-rtp → AIC RTP 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_rtp_adc.c` | RTP ADC 采样测试 |
| `test_rtp_draw.c` | RTP 触摸绘图测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_RTP_TEST` | 启用 RTP 驱动测试命令 | n | AIC_RTP_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable RTP driver test command
```

## 使用说明

### `test_rtp_adc` — RTP ADC 采样测试

**使用示例：**

```bash
# 执行 RTP ADC 采样
test_rtp_adc

```

### `test_rtp_draw` — RTP 触摸绘图测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-h, --help` | 显示帮助 |

**使用示例：**

```bash
# 启动触摸绘图模式
test_rtp_draw

```

## 依赖说明

- **上游依赖**：AIC RTP 驱动
- **下游被依赖**：无

> **注意**：RTP ADC 电压范围基于 VCCIO_3V3，GPAI ADC 基于 LDO。
