# 按键 ADC 测试 (test-keyadc)

测试按键 ADC（KEYADC）采样功能。

## 调用关系

```
Bare Shell → test-keyadc → AIC KEYADC 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_keyadc.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_KEYADC_TEST` | 启用 KEYADC 测试命令 | n | `AIC_KEYADC_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 KEYADC 测试命令
```

## 使用说明

### `test_keyadc` — KEYADC 按键测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `read <ch>` | 读取指定通道 ADC 值，通道号 [0, N]，默认 2 |
| `set <scale>` | 设置 ADC 缩放值，默认 50 |
| `start` | 启动 KEYADC 测试 |
| `help` | 显示帮助信息 |

**使用示例：**

```bash
# 读取通道 6 的值
test_keyadc read 6
# 设置缩放值为 80
test_keyadc set 80
# 启动测试
test_keyadc start
```

## 依赖说明

- **上游依赖**：AIC KEYADC 驱动
- **下游被依赖**：无
