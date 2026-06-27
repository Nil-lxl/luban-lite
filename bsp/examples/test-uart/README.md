# UART 串口测试 (test-uart)

## 一句话描述

UART 串口通信测试，验证串口的数据收发功能（自发自收回环测试）。

## 调用关系

```
MSH Shell → test-uart → AIC UART 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_uart.c` | UART 收发测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_UART_DRV_TEST` | 启用 UART 驱动测试命令 | n | AIC_UART_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable UART driver test command
```

## 使用说明

### `test_uart` — UART 串口收发测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-p, --port <name>` | 串口设备名称，默认 uart4 |
| `-h, --help` | 显示帮助 |

**使用示例：**

```bash
# 在 uart4 上进行收发测试
test_uart -p uart4

# 在 uart0 上进行收发测试
test_uart -p uart0

```

## 依赖说明

- **上游依赖**：AIC UART 驱动
- **下游被依赖**：无
