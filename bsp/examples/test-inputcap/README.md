# 输入捕获测试 (test-inputcap)

## 一句话描述

输入捕获（Input Capture）功能测试，验证定时器对脉冲宽度/频率的测量。

## 调用关系

```
MSH Shell → test-inputcap → AIC INPUTCAP 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_inputcap.c` | 输入捕获测试主程序 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_INPUTCAP_DRV_TEST` | 启用 INPUTCAP 驱动测试命令 | n | AIC_INPUTCAP_DRV && !DRIVER_BARE_DRV_EN |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable INPUTCAP driver test command
```

## 使用说明

### `test_inputcap` — 输入捕获测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<channel>` | 输入捕获通道号 |

**使用示例：**

```bash
# 测试输入捕获通道 0
test_inputcap 0

```

## 依赖说明

- **上游依赖**：AIC INPUTCAP 驱动
- **下游被依赖**：无
