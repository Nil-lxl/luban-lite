# 精密 ADC 测试 (test-psadc)

测试 PSADC（精密 Sigma-Delta ADC）采样功能。

## 调用关系

```
Bare Shell → test-psadc → AIC PSADC 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_psadc.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_PSADC_TEST` | 启用 PSADC 测试命令 | n | `AIC_PSADC_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 PSADC 测试命令
```

## 使用说明

### `test_psadc` — PSADC 采样测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `read` | 获取 ADC 值 |
| `modify <voltage>` | 修改默认参考电压 |
| `status` | 检查 PSADC 状态 |
| `help` | 显示帮助信息 |

**使用示例：**

```bash
# 读取当前 ADC 值
test_psadc read
# 修改参考电压为 3.3V
test_psadc modify 3.3
# 检查 PSADC 状态
test_psadc status
```

## 依赖说明

- **上游依赖**：AIC PSADC 驱动
- **下游被依赖**：无
