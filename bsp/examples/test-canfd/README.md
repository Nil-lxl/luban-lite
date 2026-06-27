# CAN FD 总线测试 (test-canfd)

## 一句话描述

CAN FD（Flexible Data-rate）总线收发、回环和集成测试。

## 调用关系

```
MSH Shell → test-canfd → AIC CANFD 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_canfd.c` | CAN FD 收发测试 |
| `test_canfd_loopback.c` | CAN FD 回环测试 |
| `test_canfd_common.h` | 公共头文件 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_CANFD_DRV_TEST` | 启用 CANFD 驱动测试命令 | n | AIC_CANFD_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable CANFD driver test command
```

## 使用说明

### `canfd_rx` — CAN FD 接收

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<device>` | CAN FD 设备名，如 canfd0 |

**使用示例：**

```bash
# 在 canfd0 上接收数据
canfd_rx canfd0

```

### `canfd_tx` — CAN FD 发送

**使用示例：**

```bash
# 发送 CAN FD 数据
canfd_tx

```

### `test_canfd_loopback` — CAN FD 回环测试

**使用示例：**

```bash
# 执行回环
test_canfd_loopback

```

## 依赖说明

- **上游依赖**：AIC CANFD 驱动
- **下游被依赖**：无
