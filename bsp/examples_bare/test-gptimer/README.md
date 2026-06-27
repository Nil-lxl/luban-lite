# 通用定时器测试 (test-gptimer)

测试 GPTimer 的单次触发、周期触发、计数匹配等功能。

## 调用关系

```
Bare Shell → test-gptimer → AIC GPTIMER 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_gptimer.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_GPTIMER_DRV_BARE_TEST` | 启用 GPTimer 裸机测试命令 | n | `AIC_GPTIMER_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 GPTimer 裸机测试命令
```

## 使用说明

### `test_gptimer` — GPTimer 定时器测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-m, --mode <mode>` | 定时器模式：oneshot / period |
| `-c, --channel <ch>` | 定时器通道号 [0, N] |
| `-s, --second <s>` | 秒数（必须 > 0） |
| `-u, --microsecond <us>` | 微秒数（必须 > 0） |
| `-g, --gptmode <mode>` | GP 模式：count / match |
| `-a, --trgmode <mode>` | 触发模式：auto / rsi / fall / bil |
| `-f, --frequency <Hz>` | 频率（必须 > 0） |
| `-d, --debug` | 显示超时日志 |
| `-h, --usage` | 显示帮助信息 |

**使用示例：**

```bash
# 周期模式，通道 0，1 秒 3 微秒
test_gptimer -m period -c 0 -s 1 -u 3
# 单次模式，通道 1，频率 1MHz
test_gptimer -m oneshot -c 1 -f 1000000
# 计数匹配模式，自动触发
test_gptimer -m period -c 0 -s 1 -g count -a auto
```

## 依赖说明

- **上游依赖**：AIC GPTIMER 驱动
- **下游被依赖**：无
