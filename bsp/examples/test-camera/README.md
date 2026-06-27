# 摄像头测试 (test-camera)

## 一句话描述

摄像头设备测试程序，支持通过 ioctl 命令控制摄像头参数（帧率、曝光、增益等）。

## 调用关系

```
MSH Shell → test-camera → AIC Camera 驱动 (`bsp/peripheral/camera/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_camera.c` | 摄像头 ioctl 测试主程序 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_CAMERA_DRV_TEST` | 启用摄像头驱动测试命令 | n | AIC_USING_CAMERA |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable Camera driver test command
```

## 使用说明

### `test_camera` — 摄像头 ioctl 命令测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-c, --command <cmd>` | ioctl 命令名称（如 fps、exposure、gain 等） |
| `-v, --value <val>` | 命令参数值 |
| `-l, --list` | 列出所有支持的 ioctl 命令 |
| `-h, --usage` | 显示帮助信息 |

**使用示例：**

```bash
# 列出所有支持的 ioctl 命令
test_camera -l

# 设置摄像头帧率为 20
test_camera -c "fps" -v 20

# 设置曝光值为 100
test_camera -c "exposure" -v 100

```

## 依赖说明

- **上游依赖**：AIC Camera 外设驱动, DVP 视频输入
- **下游被依赖**：无
