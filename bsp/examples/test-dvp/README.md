# DVP 视频端口测试 (test-dvp)

## 一句话描述

DVP（Digital Video Port）数字视频端口与摄像头设备测试程序。

## 调用关系

```
MSH Shell → test-dvp → AIC DVP 驱动 → Camera 外设
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_dvp.c` | DVP 与摄像头测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_DVP_TEST` | 启用 DVP 驱动测试命令 | n | AIC_DVP_DRV_V11 |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable DVP driver test command
```

## 使用说明

### `test_dvp` — DVP 摄像头测试

支持 DVP 视频采集、格式选择、帧数控制和画面显示。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-f, --format <fmt>` | 输出视频格式：`nv16` / `nv12` / `yuv400` 等 |
| `-c, --count <n>` | 采集帧数（0 表示持续采集） |
| `-a, --angle <deg>` | 旋转角度（仅在支持旋转时有效） |
| `-w, --wait` | 等待模式 |
| `-h, --usage` | 显示帮助信息 |

**使用示例：**

```bash
# NV16 格式，采集 1 帧
test_dvp -f nv16 -c 1

# NV12 格式，持续采集
test_dvp -f nv12 -c 0

# 采集 10 帧，旋转 90 度
test_dvp -f nv16 -c 10 -a 90
```


## 依赖说明

- **上游依赖**：AIC DVP V11 驱动, Camera 外设驱动
- **下游被依赖**：无
