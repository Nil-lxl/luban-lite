# 电容触摸屏测试 (test-ctp)

测试电容触摸屏（CTP）驱动功能。

## 调用关系

```
Bare Shell → test-ctp → AIC CTP 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_ctp.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_TP_BARE_TEST` | 启用电容触摸屏裸机测试命令 | n | `AIC_TP_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用电容触摸屏裸机测试命令
```

## 使用说明

### `test_ctp` — 电容触摸屏测试

初始化 GT911 触摸屏驱动，读取芯片 ID，然后进入循环不断读取触摸点数据并打印坐标。

> 执行后将持续运行，无法返回 Shell（死循环）。

**使用示例：**

```bash
# 执行 CTP 测试
test_ctp
```

## 依赖说明

- **上游依赖**：AIC CTP 驱动
- **下游被依赖**：无
