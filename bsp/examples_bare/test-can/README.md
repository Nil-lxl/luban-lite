# CAN 总线测试 (test-can)

测试 CAN 总线通信功能。

## 调用关系

```
Bare Shell → test-can → AIC CAN 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_can.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_CAN_DRV_TEST` | 启用 CAN 测试命令 | n | `AIC_CAN_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 CAN 测试命令
```

## 使用说明

### `test_can` — CAN 自环测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<CAN BUS ID>` | CAN 总线编号，如 0 / 1 |

> 注意：执行前需将 CAN TX 和 RX 引脚短接（自环模式），波特率固定 1Mbps，帧 ID 为 `0x123`。

**使用示例：**

```bash
# 测试 CAN0 自环模式
test_can 0

# 测试 CAN1 自环模式
test_can 1
```

## 依赖说明

- **上游依赖**：AIC CAN 驱动
- **下游被依赖**：无
