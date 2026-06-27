# PWM 测试 (test-pwm)

测试 PWM 输出功能，可设置通道、周期和占空比。

## 调用关系

```
Bare Shell → test-pwm → AIC PWM 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_pwm.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_PWM_TEST` | 启用 PWM 测试命令 | n | `AIC_PWM_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 PWM 测试命令
```

## 使用说明

### `test_pwm` — PWM 输出测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<ch>` | PWM 通道号 |
| `<period>` | 周期值 |
| `<duty>` | 占空比值 |

**使用示例：**

```bash
# 通道 0，周期 1000，占空比 500
test_pwm 0 1000 500
# 通道 1，周期 2000，占空比 1000
test_pwm 1 2000 1000
```

## 依赖说明

- **上游依赖**：AIC PWM 驱动
- **下游被依赖**：无
