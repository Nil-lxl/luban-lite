# 按键 ADC 测试 (test-keyadc)

## 一句话描述

KeyADC（按键 ADC）设备测试，通过 ADC 采样实现按键识别。

## 调用关系

```
MSH Shell → test-keyadc → AIC GPAI 驱动（KeyADC 功能）
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_keyadc.c` | 按键 ADC 测试主程序 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_KEYADC_TEST` | 启用 KEYADC 驱动测试命令 | n | AIC_GPAI_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable KEYADC driver test command
```

## 使用说明

### `test_keyadc` — 按键 ADC 采样测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-c, --channel <n>` | 选择 ADC 通道号 |
| `-s, --scale <n>` | 设置缩放因子 |
| `-h, --help` | 显示帮助信息 |

**使用示例：**

```bash
# 通道 0 按键采样
test_keyadc -c 0

# 通道 2 按键采样，缩放因子 10
test_keyadc -c 2 -s 10

```

## 依赖说明

- **上游依赖**：AIC GPAI 驱动
- **下游被依赖**：无
