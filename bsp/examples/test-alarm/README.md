# RTC 闹钟测试 (test-alarm)

## 一句话描述

RTC 闹钟功能测试，设置定时器在指定秒数后触发闹钟事件。

## 调用关系

```
MSH Shell → test-alarm → AIC RTC 驱动 + RT-Thread Alarm
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_alarm.c` | RTC 闹钟测试主程序 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_RTC_DRV_TEST` | 启用 RTC 驱动测试命令 | n | `AIC_RTC_DRV \|\| AIC_USING_EXTERNAL_RTC` |
| `RT_USING_ALARM` | 启用 RT-Thread 闹钟功能 | y | - |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable RTC driver test command
```

## 使用说明

### `test_alarm` — 设置 RTC 闹钟

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<timeout>` | 闹钟触发时间（秒） |

**使用示例：**

```bash
# 10 秒后触发闹钟
test_alarm 10

# 60 秒后触发闹钟
test_alarm 60

```

## 依赖说明

- **上游依赖**：AIC RTC 驱动, RT-Thread Alarm 组件
- **下游被依赖**：无
