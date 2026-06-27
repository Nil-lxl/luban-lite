# RTC 实时时钟测试 (test-rtc)

## 一句话描述

RTC（Real Time Clock）实时时钟综合测试，包含边界值测试、异常参数验证和基础功能测试。

## 调用关系

```
MSH Shell → test-rtc → AIC RTC 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_rtc.c` | RTC 基础功能测试（设置/获取时间） |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_RTC_DRV_TEST` | 启用 RTC 驱动测试命令 | n | `AIC_RTC_DRV \|\| AIC_USING_EXTERNAL_RTC` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable RTC driver test command
```

## 使用说明

### `test_rtc` — RTC 时间设置测试

设置 RTC 日期和时间，并回读验证。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `YYYY` | 年份 |
| `MM` | 月份 [1, 12] |
| `DD` | 日期 [1, 31] |
| `HH` | 小时 [0, 23] |
| `MM` | 分钟 [0, 59] |
| `SS` | 秒 [0, 59] |

**使用示例：**

```bash
# 设置时间为 2024-12-25 20:30:00
test_rtc 2024 12 25 20 30 00
```


## 依赖说明

- **上游依赖**：AIC RTC 驱动
- **下游被依赖**：test-alarm（RTC 闹钟测试依赖 RTC）
