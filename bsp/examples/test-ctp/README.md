# 电容触摸屏测试 (test-ctp)

## 一句话描述

CTP（Capacitive Touch Panel）电容触摸屏测试，验证触摸事件读取和坐标获取。

## 调用关系

```
MSH Shell → test-ctp → AIC Touch 驱动 → 外设 CTP 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_touch.c` | 电容触摸屏测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_TP_DRV_TEST` | 启用触摸屏驱动测试命令 | n | AIC_USING_TOUCH |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable touch panel driver test command
```

## 使用说明

### `test_touch` — 触摸屏测试

启动触摸设备并创建后台线程，实时打印触摸坐标信息。

**使用示例：**

```bash
# 启动触摸测试（自动检测设备）
test_touch
```


## 依赖说明

- **上游依赖**：AIC Touch 驱动, 外设 CTP 驱动（如 gt9xx、ft6x36）
- **下游被依赖**：无
