# XPWM/PWM 扩展测试 (test-xpwm)

测试 XPWM（扩展 PWM）的多种输出模式和 FIFO 功能。

## 调用关系

```
Bare Shell → test-xpwm → AIC XPWM 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_xpwm.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_XPWM_BARE_TEST` | 启用 XPWM 裸机测试命令 | n | `AIC_XPWM_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 XPWM 裸机测试命令
```

## 使用说明

### `test_xpwm` — XPWM 扩展 PWM 测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `set <ch> <period> <pulse> <pulse_cnt>` | 设置 PWM 输出参数 |
| `set_fifo_num <ch> <fifo_num>` | 设置 FIFO 数量 |
| `set_fifo <ch> <idx> <period> <pulse> <pulse_cnt>` | 设置 FIFO 条目 |
| `get_fifo <ch>` | 获取 FIFO 配置 |
| `dma_set_fifo <ch> <period> <pulse> <pulse_cnt>` | DMA 模式设置 FIFO |
| `dma_test <ch> <loop_times>` | DMA 循环测试 |

**使用示例：**

```bash
# 设置通道 0 的 PWM 参数
test_xpwm set 0 1000 500 1
# 获取通道 0 的 FIFO 配置
test_xpwm get_fifo 0
# DMA 循环测试，通道 0，10 次
test_xpwm dma_test 0 10
```

## 依赖说明

- **上游依赖**：AIC XPWM 驱动
- **下游被依赖**：无
