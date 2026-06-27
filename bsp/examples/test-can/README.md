# CAN 总线测试 (test-can)

## 一句话描述

CAN 总线收发与回环测试。标准模式需要两个 CAN 模块（CAN0 发/CAN1 收），回环模式仅需单模块。

## 调用关系

```
MSH Shell → test-can → AIC CAN 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_can.c` | CAN 收发测试（CAN0 发送，CAN1 接收） |
| `test_can_loopback.c` | CAN 回环测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_CAN_DRV_TEST` | 启用 CAN 驱动测试命令 | n | AIC_CAN_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable CAN driver test command
```

## 使用说明

### `can_rx` — CAN 接收测试

启动接收线程，打印 CAN 总线接收到的帧数据。

**使用示例：**

```bash
can_rx
```

### `can_tx` — CAN 发送测试

发送指定 ID 和数据的 CAN 帧。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `CAN_FRAME` | 格式：`frame_id#byte1.byte2...`（十六进制） |

**使用示例：**

```bash
# 发送 ID=0x1a3，数据 11.22.9a.88.ef.00
can_tx 1a3#11.22.9a.88.ef.00
```

### `test_can` — CAN 回环测试

对指定 CAN 设备执行发送+接收回环测试。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `CAN_DEV` | CAN 设备名，如 `can0`、`can1` |
| `CAN_FRAME` | 格式：`frame_id#byte1.byte2...`（十六进制） |

**使用示例：**

```bash
test_can can0 1a3#11.22.9a.88.ef.00
test_can can1 1a3#11.22.33.44.55.66.77.88
```


## 依赖说明

- **上游依赖**：AIC CAN 驱动
- **下游被依赖**：无
