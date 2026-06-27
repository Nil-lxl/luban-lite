# 高精度定时器测试 (test-hrtimer)

测试 HRTimer 的定时和 PWM 输出功能。

## 调用关系

```
Bare Shell → test-hrtimer → AIC HRTIMER 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_hrtimer.c` | 测试实现 |
| `test_hrtimer_pwm.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_HRTIMER_DRV_TEST` | 启用 HRTimer 测试命令 | n | `AIC_HRTIMER_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 HRTimer 测试命令
```

## 使用说明

### `test_hrtimer` — HRTimer 定时测试（V10/V11 版本）

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<channel>` | HRTimer 通道号 [0, N] |
| `<time>` | 定时时间，单位微秒 (us) |

> 定时器触发时会翻转配置的 GPIO 引脚作为演示。

**使用示例：**

```bash
# 通道 0，定时 20us
test_hrtimer 0 20

# 通道 1，定时 100us
test_hrtimer 1 100
```

## 依赖说明

- **上游依赖**：AIC HRTIMER 驱动
- **下游被依赖**：无
