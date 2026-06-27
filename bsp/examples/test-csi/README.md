# Xuantie CSI DSP 测试 (test-csi)

## 一句话描述

Xuantie CSI DSP 测试套件，包含 FFT 变换、FIR 滤波器、卷积等信号处理算法验证（仅 d13x 芯片）。

## 调用关系

```
MSH Shell → test-csi → CSI DSP 硬件
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_csi.c` | CSI DSP 测试主入口 |
| `test_csi_conv.c` | 卷积运算测试 |
| `test_csi_fft.c` | FFT 变换测试 |
| `test_csi_fir.c` | FIR 滤波器测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_CSI_DSP_TEST` | 启用 CSI DSP 测试命令 | n | AIC_CHIP_D13X |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable CSI DSP test command
```

## 使用说明

### `test_csi` — CSI DSP 测试套件

支持 FFT、FIR、卷积等 DSP 运算测试。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `fft` | 执行 FFT 测试 |
| `fir` | 执行 FIR 滤波器测试 |
| `conv` | 执行卷积测试 |
| `all` | 执行全部测试 |

**使用示例：**

```bash
# 仅 FFT 测试
test_csi fft

# 仅 FIR 测试
test_csi fir

# 执行全部测试
test_csi all
```


## 依赖说明

- **上游依赖**：d13x 芯片 CSI DSP 模块
- **下游被依赖**：无
