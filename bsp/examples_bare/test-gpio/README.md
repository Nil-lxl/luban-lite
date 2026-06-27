# GPIO 测试 (test-gpio)

测试 GPIO 输入、输出和中断功能。

## 调用关系

```
Bare Shell → test-gpio → AIC GPIO 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_gpio.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_GPIO_DRV_TEST` | 启用 GPIO 测试命令 | n | `AIC_GPIO_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 GPIO 测试命令
```

## 使用说明

### `test_gpio` — GPIO 输入/输出测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `input <PIN>` | 配置引脚为输入模式，打印按下计数，默认 PD.15 |
| `output <PIN>` | 配置引脚为输出模式 |
| `check <PIN>` | 检查引脚配置状态 |
| `help` | 显示帮助信息 |

**使用示例：**

```bash
# 配置 PD.4 为输入模式
test_gpio input PD.4
# 配置 PA.0 为输出模式
test_gpio output PA.0
# 检查 PD.4 配置状态
test_gpio check PD.4
```

## 依赖说明

- **上游依赖**：AIC GPIO 驱动
- **下游被依赖**：无
