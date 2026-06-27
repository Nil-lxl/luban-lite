# WiFi 无线测试 (test-wifi)

## 一句话描述

WiFi 无线网络综合测试，支持 STA/AP 模式基础测试、iperf 性能和压力测试。

## 调用关系

```
MSH Shell → test-wifi → AIC WiFi 驱动 → 无线外设
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_wifi.c` | WiFi 综合测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_WIFI_TEST` | 启用 WiFi 测试命令 | n | AIC_WIRELESS_LAN && !DRIVER_BARE_DRV_EN |
| `OPEN_NETWORK_SSID` | 开放网络 SSID（STA 测试用） | "nopassword" | AIC_WIFI_TEST |
| `WPA2_AES_PSK_NETWORK_SSID` | WPA2 加密网络 SSID | "my-test" | AIC_WIFI_TEST |
| `WPA2_AES_PSK_NETWORK_PASSWORD` | WPA2 加密网络密码 | "12345678" | AIC_WIFI_TEST |
| `AP_TEST_SSID` | AP 模式测试 SSID | "WiFi-test" | AIC_WIFI_TEST |
| `AP_TEST_PASSWORD` | AP 模式测试密码 | "12345678" | AIC_WIFI_TEST |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable Wi-Fi test command
```

## 使用说明

### `test_wifi` — WiFi 综合测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-s, --sta_test [dev]` | STA 模式基础功能测试（可选指定设备名） |
| `-a, --ap_test [dev]` | AP 模式基础功能测试（可选指定设备名） |
| `-p, --ap_tcp [server]` | AP 模式 iperf TCP 服务器/客户端 |
| `-t, --sta_tcp [server]` | STA 模式 iperf TCP 服务器/客户端 |
| `-b, --ap_udp [server]` | AP 模式 iperf UDP 服务器 |
| `-c, --sta_udp [server]` | STA 模式 iperf UDP 服务器/客户端 |
| `-x, --stress` | AP/STA 模式无限循环切换压力测试 |
| `-d, --del_stress` | 删除压力测试线程 |
| `-l, --load` | 创建 CPU 负载线程 |
| `-k, --del_load` | 删除负载线程 |
| `-h, --usage` | 显示帮助 |

**使用示例：**

```bash
# STA 模式基础测试
test_wifi -s

# 指定 wlan0 进行 STA 测试
test_wifi -swlan0

# AP 模式基础测试
test_wifi -a

# AP 模式 iperf TCP 测试
test_wifi -p

# STA 模式连接指定 iperf 服务器
test_wifi -t server

# AP/STA 循环切换压力测试
test_wifi -x

```

## 依赖说明

- **上游依赖**：AIC WiFi 驱动, AIC 无线外设, lwIP 协议栈
- **下游被依赖**：无
