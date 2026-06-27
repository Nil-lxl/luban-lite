# 温度传感器测试 (test-tsen)

测试片上温度传感器（TSensor）采样功能。

## 调用关系

```
Bare Shell → test-tsen → AIC TSEN 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_tsen.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_TSEN_TEST` | 启用温度传感器测试命令 | n | `AIC_TSEN_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用温度传感器测试命令
```

## 使用说明

### `test_tsen` — 温度传感器测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `read <count>` | 读取温度值，采样次数 |
| `help` | 显示帮助信息 |

**使用示例：**

```bash
# 读取 1 次温度值
test_tsen read 1
# 连续读取 10 次温度值
test_tsen read 10
```

## 依赖说明

- **上游依赖**：AIC TSEN 驱动
- **下游被依赖**：无
