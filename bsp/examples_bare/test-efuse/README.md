# eFuse 测试 (test-efuse)

测试 eFuse（电子熔丝）的读写和 SPIENC 密钥烧录功能。

## 调用关系

```
Bare Shell → test-efuse → AIC EFUSE 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `efuse_burn_spienc_key_cmd.c` | 测试实现 |
| `efuse_cmds.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_SID_BARE_TEST` | 启用 eFuse 裸机测试命令 | n | `AIC_SID_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 eFuse 裸机测试命令
```

## 使用说明

### `efuse` — eFuse 读写操作

**子命令说明：**

| 子命令 | 说明 |
|--------|------|
| `help` | 显示帮助信息 |
| `dump <offset> <len>` | 从 eFuse 偏移地址 dump 数据，十六进制显示 |
| `read <addr> <offset> <len>` | 从 eFuse 偏移地址读取数据到 RAM |
| `write <addr> <offset> <len>` | 从 RAM 地址写入数据到 eFuse（需 `EFUSE_WRITE_SUPPORT`） |
| `writehex <offset> <data>` | 从十六进制字符串写入 eFuse（需 `EFUSE_WRITE_SUPPORT`） |
| `writestr <offset> <data>` | 从字符串写入 eFuse（需 `EFUSE_WRITE_SUPPORT`） |
| `authenticate sjtag <key>` | 安全 JTAG 认证（hex 字符串密钥） |
| `authenticate szone <key>` | 安全区域认证（hex 字符串密钥） |

> 所有地址和长度参数均为十六进制。

**使用示例：**

```bash
# Dump eFuse 偏移 0x00 起的 0x100 字节
efuse dump 0x00 0x100

# 读取 eFuse 偏移 0x10 起的 0x20 字节到 RAM 0x30000000
efuse read 0x30000000 0x10 0x20

# 从 RAM 0x30000000 写入 0x10 字节到 eFuse 偏移 0x10
efuse write 0x30000000 0x10 0x10

# 直接以 hex 字符串写入 eFuse
efuse writehex 0x20 0102030405

# 安全 JTAG 认证
efuse authenticate sjtag 0102030405060708
```

### `efuse_spienc` — eFuse SPIENC 密钥烧录

一次性烧录 SPI 加密启动所需的全部密钥和标志位，包括：
- SPIENC AES 密钥（16 字节）
- SPIENC NONCE 密钥（8 字节，仅 D13x/d21x/g73x）
- ROTPK 公钥哈希（16 字节，仅 d13x/d21x/g73x）
- BROM SPIENC 启动位
- JTAG 锁定位（非 DEBUG 模式）
- 密钥读写禁止位（非 DEBUG 模式）

> **警告：** 此命令为一次性烧录操作，烧录后不可逆。烧录完成后所有密钥的 CRC32 校验值将打印到控制台，并尝试保存到 SD 卡的 `burn.log` 文件。

**使用示例：**

```bash
# 烧录 SPIENC 密钥到 eFuse
efuse_spienc
```

## 依赖说明

- **上游依赖**：AIC EFUSE 驱动
- **下游被依赖**：无
