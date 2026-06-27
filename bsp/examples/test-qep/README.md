# QEP 编码器测试 (test-qep)

## 一句话描述

QEP（Quadrature Encoder Pulse）正交编码器测试，验证编码器位置/速度检测功能。

## 调用关系

```
MSH Shell → test-qep → AIC QEP 驱动 + PWM 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_qep.c` | QEP 编码器测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_QEP_DRV_TEST` | 启用 QEP 驱动测试命令 | n | AIC_QEP_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable QEP driver test command
```

## 使用说明

### `test_qep` — QEP 编码器测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<qep_channel>` | QEP 通道号 |
| `<pwm_channel>` | PWM 通道号（用于模拟编码器信号） |
| `<period_ns>` | PWM 周期（纳秒） |
| `<duty_ns>` | PWM 占空比（纳秒） |
| `<pulse_count>` | 脉冲计数 |

**使用示例：**

```bash
# QEP 通道 0，PWM 通道 1，周期 1000ns，占空比 500ns，100 个脉冲
test_qep 0 1 1000 500 100

```

## 依赖说明

- **上游依赖**：AIC QEP 驱动, AIC PWM 驱动
- **下游被依赖**：无
