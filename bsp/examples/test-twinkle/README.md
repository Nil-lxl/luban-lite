# LED 闪灯测试 (test-twinkle)

## 一句话描述

LED 闪灯（Twinkle）测试程序，通过 GPIO 控制指定引脚的 LED 亮灭闪灯。

## 调用关系

```
MSH Shell → test-twinkle → AIC GPIO 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_twinkle.c` | LED 闪灯测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_GPIO_TWINKLE_TEST` | 启用 GPIO TWINKLE 测试命令 | n | AIC_GPIO_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable GPIO TWINKLE test command
```

## 使用说明

### `test_twinkle` — LED 闪灯测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-p, --pin <name>` | GPIO 引脚名称 |
| `-n, --num <n>` | 闪灯次数 |
| `-h, --help` | 显示帮助 |

**使用示例：**

```bash
# PA.0 引脚闪灯 10 次
test_twinkle -p PA.0 -n 10

```

## 依赖说明

- **上游依赖**：AIC GPIO 驱动
- **下游被依赖**：无
