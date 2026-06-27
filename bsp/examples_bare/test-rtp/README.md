# 电阻触摸屏测试 (test-rtp)

测试 RTP（电阻触摸屏）的校准和绘图功能。

## 调用关系

```
Bare Shell → test-rtp → AIC RTP 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_rtp_draw.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_RTP_TEST` | 启用 RTP 测试命令 | n | `AIC_RTP_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 RTP 测试命令
```

## 使用说明

### `test_rtp_draw` — 电阻触摸屏绘图测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `calibrate` | 执行屏幕校准 |
| `set <points>` | 设置绘图点数，默认 1000 |
| `draw` | 绘制图形 |

**使用示例：**

```bash
# 执行屏幕校准
test_rtp_draw calibrate
# 设置绘图点数为 500
test_rtp_draw set 500
# 绘制图形
test_rtp_draw draw
```

## 依赖说明

- **上游依赖**：AIC RTP 驱动
- **下游被依赖**：无
