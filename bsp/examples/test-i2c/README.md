# I2C 从设备测试 (test-i2c)

## 一句话描述

I2C 从设备（Slave）模式测试，验证 I2C 从设备的收发通信功能。

## 调用关系

```
MSH Shell → test-i2c → AIC I2C 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_i2c_slave.c` | I2C 从设备通信测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_I2C_DRV_TEST` | 启用 I2C 驱动测试命令 | n | AIC_I2C_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable I2C driver test command
```

## 使用说明

### `test_i2c_slave` — I2C 从设备测试

将 I2C 总线配置为从设备模式，接收主设备的数据并响应。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `[i2c_name]` | I2C 总线名（可选，默认使用板级配置） |

**使用示例：**

```bash
# 使用默认 I2C 总线
test_i2c_slave

# 指定 I2C 总线
test_i2c_slave i2c1
```


## 依赖说明

- **上游依赖**：AIC I2C 驱动
- **下游被依赖**：无
