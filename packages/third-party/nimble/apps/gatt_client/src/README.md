GATT client demo的文件结构：

```
packages/third-party/nimble/apps/gatt_client/
└── src/
    ├── gatt_client.h    # 头文件，定义数据结构和函数声明
    ├── gatt_client.c    # 主程序，实现 GATT client 功能
    ├── peer.c           # 服务发现管理
    └── misc.c           # 辅助函数
```

## 功能说明

这个 demo 实现了完整的 GATT client 功能，包括：

### 1. 扫描和连接
- 扫描周围的 BLE 设备
- 通过地址连接指定设备

### 2. 服务发现
- 发现所有服务
- 发现所有特征
- 发现所有描述符

### 3. 特征操作
- 读取特征值
- 写入特征值

### 4. 描述符操作
- 读取描述符
- 写入描述符
- 启用/禁用通知
- 启用/禁用指示

## 命令列表

| 命令 | 说明 |
|------|------|
| `gatt_client` | 启动 GATT client demo（自动扫描） |
| `gattc_scan` | 开始扫描 BLE 设备 |
| `gattc_connect <addr>` | 连接到指定地址的设备（格式：XX:XX:XX:XX:XX:XX） |
| `gattc_disconnect` | 断开当前连接 |
| `gattc_discover` | 发现所有服务/特征/描述符 |
| `gattc_show` | 显示已发现的服务和特征 |
| `gattc_read_chr <handle>` | 读取特征值 |
| `gattc_write_chr <handle> <hex_data>` | 写入特征值 |
| `gattc_read_dsc <handle>` | 读取描述符 |
| `gattc_write_dsc <handle> <hex_data>` | 写入描述符 |
| `gattc_enable_notify <cccd_handle>` | 启用通知 |
| `gattc_enable_indicate <cccd_handle>` | 启用指示 |

## 使用示例

```bash
# 1. 启动扫描
gattc_scan

# 2. 连接到设备（使用扫描到的地址）
gattc_connect 11:22:33:44:55:66

# 3. 连接成功后，自动进行服务发现
# 也可以手动执行：
gattc_discover

# 4. 查看发现的服务和特征
gattc_show

# 5. 读取特征值
gattc_read_chr 0x0012

# 6. 写入特征值
gattc_write_chr 0x0012 01020304

# 7. 读取描述符
gattc_read_dsc 0x0013

# 8. 启用通知（写入 CCCD）
gattc_enable_notify 0x0013

# 9. 断开连接
gattc_disconnect
```

## 与心率 demo 的区别

- **心率 demo (blehr)**：设备作为 **GATT Server**，等待手机连接和订阅心率数据
- **此 demo (gatt_client)**：设备作为 **GATT Client**，主动连接其他 BLE 设备并进行 GATT 操作

这个 demo 可以用来验证板端的 GATT client 接口是否正常工作。
