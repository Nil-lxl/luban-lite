# 多路 PWM 测试 (test-multipwm)

## 一句话描述

多路 PWM（EPWM 和普通 PWM）输出测试，验证多通道 PWM 的周期与占空比配置。

## 调用关系

```
MSH Shell → test-multipwm → RT-Thread PWM 框架 → AIC PWM 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_multipwm.c` | 多路 PWM 测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_PWM_TEST` | 启用 PWM 驱动测试命令 | n | `RT_USING_PWM \|\| DRIVER_BARE_DRV_EN` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable PWM driver test command
```

## 使用说明

### `test_multipwm` — 多路 PWM/EPWM 测试

演示多路 PWM 和 EPWM 输出配置，包括 LED 亮度控制、机械臂控制、云台电机等场景。

**使用示例：**

```bash
# 启动多路 PWM/EPWM 输出
test_multipwm
```


## 依赖说明

- **上游依赖**：RT-Thread PWM 框架, AIC PWM/EPWM 驱动
- **下游被依赖**：无
