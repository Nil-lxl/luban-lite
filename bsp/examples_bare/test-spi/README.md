# SPI 测试 (test-spi)

测试 SPI 收发功能。

## 调用关系

```
Bare Shell → test-spi → AIC SPI 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_spi.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_QSPI_DRV_TEST` | 启用 QSPI 测试命令 | n | `AIC_QSPI_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 QSPI 测试命令
```

## 使用说明

### `test_spi` — SPI 收发测试

**子命令说明：**

| 子命令 | 说明 |
|--------|------|
| `init <bus_id>` | 初始化 SPI 总线，指定总线 ID |
| `send <addr> <size>` | 从指定地址发送数据，显示 hexdump |
| `recv <len>` | 接收指定长度的数据，显示 hexdump |

**使用示例：**

```bash
# 初始化 SPI 总线 0
test_spi init 0

# 分配缓冲区并填充测试数据
mtd alloc 4096
# 假设 Alloc buffer: 0x300422c0
mtd_gendata 0x300422c0 0x100 0xA0

# 发送数据
test_spi send 0x300422c0 0x100

# 接收数据
test_spi recv 0x100
```

## 依赖说明

- **上游依赖**：AIC SPI 驱动
- **下游被依赖**：无
