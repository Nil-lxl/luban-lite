# test-pm 使用说明

本目录包含 ArtInChip SoC 的电源管理（PM）测试用例。

## 编译条件

在 SDK 配置中开启对应的宏，文件才会被编译进固件：

| 宏定义 | 编译的文件 |
|--------|-----------|
| `AIC_PM_DEMO` | `pm_demo.c` |
| `AIC_PM_TEST` | `test_pm.c`、`test_pm_mbox.c` |
| `AIC_PM_STRESS_TEST` | `pm_stress.c` |

> 以上均依赖 `RT_USING_FINSH` 已开启。

---

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_PM_DEMO` | 启用 PM 示例 | n | AIC_PM_DRV |
| `AIC_PM_TEST` | 启用 PM 测试 | n | AIC_PM_DRV |
| `AIC_PM_STRESS_TEST` | 启用 PM 压力测试 | n | AIC_PM_DRV && AIC_RTC_DRV_V121 |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Power Management examples
```

## 1. pm_demo.c — 交互式电源管理演示

**功能**：上电自动运行，支持按键或触摸屏唤醒/进入睡眠。

**使用方式**：无需手动输入命令，开机后自动生效。按电源键（由 `AIC_PM_POWER_KEY_GPIO` 定义）可切换唤醒/睡眠状态。

---

## 2. test_pm.c — 基础稳定性测试

**功能**：通过低功耗定时器，每隔固定秒数自动切换一次唤醒/睡眠。

**命令格式**：
```
test_pm <秒数>
```

**示例**：
```
test_pm 10
```
系统每隔 10 秒交替进入睡眠和唤醒，可用于长时间稳定性测试。

---

## 3. pm_stress.c — 深度睡眠压力测试

**功能**：通过 RTC 闹钟定时唤醒，循环进入深度睡眠，验证唤醒可靠性。

**命令格式**：
```
pm_stress <循环次数> <间隔秒数>
```

- `循环次数`：`0` 表示无限循环；正整数表示循环指定次数后自动停止。
- `间隔秒数`：每次进入深度睡眠前等待的秒数，同时也是 RTC 闹钟的间隔。

**示例**：
```
pm_stress 0 1
```
每隔 1 秒进入一次深度睡眠，无限循环。

**停止命令**：
```
pm_stress_stop
```
随时输入即可停止正在运行的压力测试。

---

## 4. test_pm_mbox.c — 多核邮箱测试

**功能**：在从核上运行 RPMsg-lite 端点，接收主核通过 mailbox 发来的电源管理指令。

**命令格式**：
```
pm_mbox <link_name>
```

**示例**：
```
pm_mbox my_link
```
从核等待主核连接，响应 `RPMSG_CMD_IS_IDLE` 和 `RPMSG_CMD_PRE_STANDBY` 指令。

---

## 快速参考

| 命令 | 作用 |
|------|------|
| `test_pm 10` | 每 10 秒切换一次睡眠/唤醒 |
| `pm_stress 0 5` | 每 5 秒进入深度睡眠，无限循环 |
| `pm_stress_stop` | 停止压力测试 |
| `pm_mbox my_link` | 启动邮箱通信，等待主核指令 |
