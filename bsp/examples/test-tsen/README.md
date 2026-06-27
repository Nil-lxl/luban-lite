# 温度传感器测试 (test-tsen)

## 一句话描述

TSEN（Temperature Sensor）温度传感器测试，支持基础温度采样和高温报警模式。

## 调用关系

```
MSH Shell → test-tsen → AIC TSEN 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_tsen.c` | 温度传感器基础测试 |
| `test_tsen_htp.c` | 高温报警模式测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_TSEN_TEST` | 启用 TSEN 驱动测试命令 | n | AIC_TSEN_DRV |
| `AIC_TSEN_HIGH_TEMP_ALARM_TEST` | 启用高温报警测试 | n | AIC_TSEN_DRV |
| `AIC_TSEN_HIGH_TEMP_ALARM_THD` | 高温报警阈值（°C） | 75 | AIC_TSEN_HIGH_TEMP_ALARM_TEST |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable TSEN driver test command / Enable HIGH TEMPERATURE ALARM test command
```

## 使用说明

### `test_tsen` — 温度传感器采样测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-s, --sensor_id <n>` | 传感器 ID |
| `-c, --samp_cnt <n>` | 采样次数 |
| `-p, --polling_time <ms>` | 轮询间隔（ms） |
| `-r, --record_time <s>` | 记录时间（秒） |
| `-t, --temp_diff_th <n>` | 温度差异阈值 |
| `-m, --mode_id <n>` | 工作模式 ID |
| `-h, --help` | 显示帮助 |

**使用示例：**

```bash
# 采样 100 次
test_tsen -c 100

# 传感器 0，50 次，间隔 1 秒
test_tsen -s 0 -c 50 -p 1000

```

## 依赖说明

- **上游依赖**：AIC TSEN 驱动
- **下游被依赖**：无
