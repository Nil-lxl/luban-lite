# 输入捕获测试 (test-inputcap)

测试输入捕获功能，测量 PWM 信号频率和占空比。

## 调用关系

```
Bare Shell → test-inputcap → AIC INPUTCAP 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_inputcap.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_INPUTCAP_DRV_BARE_TEST` | 启用输入捕获裸机测试命令 | n | `AIC_INPUTCAP_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用输入捕获裸机测试命令
```

## 使用说明

### `test_inputcap` — 输入捕获测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<channel>` | 输入捕获通道号 |

**使用示例：**

```bash
# 在通道 0 上执行输入捕获测试
test_inputcap 0
```

## 依赖说明

- **上游依赖**：AIC INPUTCAP 驱动
- **下游被依赖**：无
