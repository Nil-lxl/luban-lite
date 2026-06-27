# GPIO 测试 (test-gpio)

## 一句话描述

GPIO 设备测试，验证引脚读写和按键中断功能。

## 调用关系

```
MSH Shell → test-gpio → AIC GPIO 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_gpio.c` | GPIO 设备读写测试 |
| `test_gpio_key.c` | GPIO 按键中断测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_GPIO_DRV_TEST` | 启用 GPIO 驱动测试命令 | n | AIC_GPIO_DRV |
| `AIC_GPIO_KEY_TEST` | 启用 GPIO KEY 测试命令 | n | AIC_GPIO_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable GPIO driver test command / Enable GPIO KEY test command
```

## 使用说明

### `test_gpio` — GPIO 设备读写测试

支持输入引脚中断计数、输出引脚电平翻转和引脚配置状态查询。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-i, --input [pin]` | 配置为输入引脚（上拉 + 双边沿中断），默认 `PD.15` |
| `-o, --output <pin>` | 配置为输出引脚，执行高低电平翻转 |
| `-c, --check [pin]` | 查询引脚配置状态（功能、中断模式、上下拉、驱动能力、输入/输出使能） |
| `-t, --time <ms>` | 输出翻转时间间隔（毫秒），默认 `10` |
| `-h, --help` | 显示帮助信息 |

**使用示例：**

```bash
# 使用默认引脚 PD.15 作为输入（按键测试）
test_gpio -i

# 指定 PD.4 作为输入引脚
test_gpio -i PD.4

# 查询 PD.4 的当前配置状态
test_gpio -c PD.4

# 输出引脚 PD.5 与输入引脚 PD.4 对环测试（输出翻转 200ms，触发输入中断）
test_gpio -t 200 -i PD.4 -o PD.5
```

### `test_gpio_key` — GPIO 按键中断测试

支持按键短按/长按识别，通过中断计时判断按压时长。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-i, --input [pin]` | 配置为按键输入引脚（上拉 + 双边沿中断），默认 `PA.2` |
| `-t, --time <ms>` | 设置长按判定时间（毫秒），默认 `3000` |
| `-h, --help` | 显示帮助信息 |

**使用示例：**

```bash
# 使用默认引脚 PA.2，长按 3 秒判定
test_gpio_key -i

# 指定 PB.3 为按键引脚，长按判定时间 2 秒
test_gpio_key -i PB.3 -t 2000
```

## 依赖说明

- **上游依赖**：AIC GPIO 驱动
- **下游被依赖**：无
