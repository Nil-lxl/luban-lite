# 通用 ADC 输入测试 (test-gpai)

测试 GPAI（通用模拟输入）通道采样功能。

## 调用关系

```
Bare Shell → test-gpai → AIC GPAI 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_gpai.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_GPAI_TEST` | 启用 GPAI 测试命令 | n | `AIC_GPAI_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 GPAI 测试命令
```

## 使用说明

### `test_gpai` — GPAI 通道采样测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `read <ch>` | 读取指定通道 ADC 值，通道号 [0, N]，默认 0 |
| `modify <voltage>` | 修改默认参考电压 |
| `set <samples>` | 设置采样次数，默认 100 |
| `help` | 显示帮助信息 |

**使用示例：**

```bash
# 读取通道 4 的 ADC 值
test_gpai read 4
# 设置采样次数为 200
test_gpai set 200
# 修改参考电压为 3.3V
test_gpai modify 3.3
```

## 依赖说明

- **上游依赖**：AIC GPAI 驱动
- **下游被依赖**：无
