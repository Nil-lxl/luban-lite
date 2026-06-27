# I2C 工具测试 (test-i2c)

I2C 读写工具，支持 8 位和 16 位寄存器宽度。

## 调用关系

```
Bare Shell → test-i2c → AIC I2C 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_i2c.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_I2C_DRV_TEST` | 启用 I2C 测试命令 | n | `AIC_I2C_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 I2C 测试命令
```

## 使用说明

### `i2c` — I2C 读写操作

**参数说明：**

| 参数 | 说明 |
|------|------|
| `write <bus> <addr> <-8\|-16> <reg> <data>` | 写入一个字节 |
| `read <bus> <addr> <-8\|-16> <reg>` | 读取一个字节 |

**使用示例：**

```bash
# 16 位寄存器宽度写入
i2c write 0 0x50 -16 0x1234 0x11
# 16 位寄存器宽度读取
i2c read 0 0x50 -16 0x1234
# 8 位寄存器宽度写入
i2c write 0 0x50 -8 0x00 0x11
# 8 位寄存器宽度读取
i2c read 0 0x50 -8 0x00
```

## 依赖说明

- **上游依赖**：AIC I2C 驱动
- **下游被依赖**：无
