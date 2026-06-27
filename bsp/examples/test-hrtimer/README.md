# 高精度定时器测试 (test-hrtimer)

## 一句话描述

High Resolution Timer（高精度定时器）测试，验证高精度计时与中断触发。

## 调用关系

```
MSH Shell → test-hrtimer → AIC HRTimer 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_hrtimer.c` | 高精度定时器测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_HRTIMER_DRV_TEST` | 启用 HRTimer 驱动测试命令 | n | AIC_HRTIMER_DRV |
| `GPIO_TEST_PIN` | HRTimer 翻转测试引脚（仅 baremetal） | "PA.4" | KERNEL_BAREMETAL |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable HRTimer driver test command
```

## 使用说明

### `test_hrtimer` — 高精度定时器测试

支持周期/单次模式、多通道、频率配置等高精度定时器功能验证。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-m, --mode <mode>` | 定时器模式：`oneshot` / `period` |
| `-c, --channel <n>` | 通道号 [0, 2] |
| `-s, --second <n>` | 定时秒数（> 0） |
| `-u, --microsecond <n>` | 定时微秒数（> 0） |
| `-f, --frequency <n>` | 频率（Hz，> 0） |
| `-d, --debug` | 开启超时日志输出 |
| `-h, --usage` | 显示帮助信息 |

**使用示例：**

```bash
# 单次模式，通道 0，2 秒 3 微秒
test_hrtimer -m oneshot -c 0 -s 2 -u 3

# 周期模式，通道 1，100ms，开启日志
test_hrtimer -m period -c 1 -u 100000 -d
```


## 依赖说明

- **上游依赖**：AIC HRTimer 驱动
- **下游被依赖**：无
