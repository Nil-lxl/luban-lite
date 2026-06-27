# 看门狗测试 (test-wdt)

## 一句话描述

WDT（Watchdog Timer）看门狗定时器测试，包含集成测试和基础超时重启测试。

## 调用关系

```
MSH Shell → test-wdt → AIC WDT 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_wdt.c` | 看门狗基础测试（超时重启） |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_WDT_DRV_TEST` | 启用 WDT 驱动测试命令 | n | AIC_WDT_DRV && !DRIVER_BARE_DRV_EN |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable WDT driver test command
```

## 使用说明

### `test_wdt` — 看门狗超时重启测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-s, --timeout <sec>` | 看门狗超时时间（秒） |
| `-p, --pretimeout <sec>` | 预超时时间（秒） |
| `-c, --clear <n>` | 清除阈值 |
| `-w, --wreg <0|1>` | 寄存器切换开关 |
| `-g, --get` | 获取当前配置 |
| `-k, --keep` | 保持喂狗 |
| `-r, --reboot` | 触发重启 |
| `-h, --help` | 显示帮助 |

**使用示例：**

```bash
# 设置 10 秒超时
test_wdt -s 10

# 超时 10 秒，预超时 5 秒
test_wdt -s 10 -p 5

# 获取当前看门狗配置
test_wdt -g

```

## 依赖说明

- **上游依赖**：AIC WDT 驱动
- **下游被依赖**：无
