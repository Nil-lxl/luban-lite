# RTC 实时时钟测试 (test-rtc)

测试 RTC 实时时钟的读写功能。

## 调用关系

```
Bare Shell → test-rtc → AIC RTC 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_rtc.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_RTC_DRV_TEST` | 启用 RTC 测试命令 | n | `AIC_RTC_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 RTC 测试命令
```

## 使用说明

### `date` — RTC 日期操作

**参数说明：**

| 参数 | 说明 |
|------|------|
| 无参数 | 显示当前日期和时区 |
| `<year> <month> <day> <hour> <min> <sec>` | 设置 RTC 时间 |

> 年：1900+，月：[1,12]，日：[1,31]，时：[0,23]，分：[0,59]，秒：[0,60]

**使用示例：**

```bash
# 显示当前日期和时间
date

# 设置时间为 2024 年 1 月 15 日 10:30:00
date 2024 01 15 10 30 00
```

## 依赖说明

- **上游依赖**：AIC RTC 驱动
- **下游被依赖**：无
