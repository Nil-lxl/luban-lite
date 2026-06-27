# 网络测试 (test-lwip)

## 一句话描述

基于 lwIP 协议栈的网络测试程序集，包含以太网回环、TCP/UDP 通信、iperf 性能测试和 MQTT 协议测试。

## 调用关系

```
MSH Shell → test-lwip/* → lwIP 协议栈 → 以太网/WiFi 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test-eth/` | 以太网回环测试 |
| `test-tcp/` | TCP 服务端回环测试 |
| `test-udp/` | UDP 单播/组播/广播回环测试 |
| `test-iperf/` | iperf 网络带宽性能测试 |
| `test_mqtt/` | MQTT 协议测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_ETH_LOOPBACK_LWIP_TEST` | 启用以太网回环测试 | n | RT_USING_LWIP |
| `AIC_TCP_LWIP_TEST` | 启用 TCP 回环测试 | n | RT_USING_LWIP |
| `AIC_UDP_LWIP_TEST` | 启用 UDP 回环测试 | n | RT_USING_LWIP |
| `AIC_IPERF_LWIP_TEST` | 启用 iperf 性能测试 | n | RT_USING_LWIP |
| `AIC_MQTT_LWIP_TEST` | 启用 MQTT 协议测试 | n | RT_USING_LWIP |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ LwIP examples
```

## 使用说明

### `test_eth` — 以太网回环测试（test-eth/）

通过 UDP 广播进行以太网回环测试，验证收发链路。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-d, --delay <ms>` | 收发间隔延迟（毫秒） |
| `-n, --number <n>` | 测试次数 |
| `-c, --circle` | 循环测试模式 |
| `-h, --help` | 显示帮助信息 |

**使用示例：**

```bash
# 延迟 300ms，测试 10 次
test_eth -d 300 -n 10

# 循环测试
test_eth -c
```

### `test_tcp` — TCP 服务端回环测试（test-tcp/）

启动 TCP Echo Server，将收到的数据原样回送（无参数，只能启动一次）。

```bash
test_tcp
```

### `test_udp` — UDP 回环测试（test-udp/）

同时启动 UDP 单播、组播、广播三路 Echo 线程（无参数，只能启动一次）。

```bash
test_udp
```

### `iperf` — 网络带宽性能测试（test-iperf/）

基于 lwIP lwiperf 的 TCP 带宽测试工具。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-s` | 作为服务端启动 |
| `-c <server_ip>` | 作为客户端连接到指定服务端 IP |

**使用示例：**

```bash
# 启动 iperf 服务端
iperf -s

# 客户端连接到 192.168.1.100
iperf -c 192.168.1.100
```

### `test_mqtt` — MQTT 协议测试（test_mqtt/）

支持 MQTT 连接、订阅、发布、取消订阅和断开连接。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-c <broker_ip>` | 连接到 MQTT Broker |
| `-s <topic> <type>` | 订阅主题（type: 0/1/2 对应 QoS 级别） |
| `-u <topic>` | 取消订阅 |
| `-p <topic> <payload>` | 发布消息到主题 |
| `-q` | 断开连接并退出 |

**使用示例：**

```bash
# 连接到 MQTT Broker
test_mqtt -c 192.168.1.100

# 订阅主题，QoS 0
test_mqtt -s sensor/temp 0

# 发布消息
test_mqtt -p sensor/temp "25.6"

# 取消订阅
test_mqtt -u sensor/temp

# 断开连接
test_mqtt -q
```

## 依赖说明

- **上游依赖**：lwIP 协议栈, 以太网/WiFi 驱动
- **下游被依赖**：无
