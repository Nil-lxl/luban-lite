# 视频输入测试程序 (test-vin)

一个通用的视频输入测试应用程序，支持 DVP、CSI 等多种视频输入设备。

## 概述

本测试程序为各种视频输入设备提供统一的测试接口，主要特性包括：

- **设备抽象层**：支持 DVP、CSI 等多种视频输入设备
- **多通道支持**：最多支持 2 个通道，每个通道有独立的上下文管理
- **拼接模式支持**：支持垂直和水平拼接模式
- **灵活配置**：命令行参数风格与 Linux 版本相似

## 文件结构

```
test-vin/
├── test_vin_dev.h    # 设备抽象层头文件
├── test_vin_dev.c    # 设备抽象层实现
├── test_vin_dvp.c    # DVP 设备驱动实现
├── test_vin.c        # 主测试程序
├── SConscript        # 编译脚本
└── README.md         # 本文件
```

## 设计说明

### 设备抽象层

设备抽象层为不同的视频输入设备提供统一接口：

```c
struct vin_dev_ops {
    int (*init)(void *priv);
    void (*deinit)(void *priv);
    int (*get_fmt)(void *priv, struct vin_video_fmt *fmt);
    int (*set_fmt)(void *priv, struct vin_video_fmt *fmt);
    int (*req_bufs)(void *priv, u32 *count);
    int (*qbuf)(void *priv, u32 index);
    int (*dqbuf)(void *priv, u32 *index);
    int (*stream_on)(void *priv);
    int (*stream_off)(void *priv);
    // ...
};
```

### 多通道支持

每个通道都有独立的上下文和状态管理：

```c
struct vin_channel {
    u32 id;
    struct vin_video_buf vbuf;
    void *priv;
};

struct vin_device {
    enum vin_dev_type type;
    const char *name;
    const char *camera_name;
    struct vin_dev_ops *ops;
    u32 num_channels;
    void *priv;
    enum vin_state state;
    struct vin_video_fmt fmt;
    struct mpp_rect crop;
    struct vin_channel channels[VIN_MAX_CHANNELS];
};
```

## 使用方法

### 命令行参数

```
test_vin [选项]

选项：
  -d, --device <dev>     视频输入设备 (dvp/csi)，默认：dvp
  -C, --channel <n>      通道号 (0-1)，默认：0
  -f, --format <fmt>     像素格式 (nv12/nv16/yuv400)，默认：nv16
  -c, --count <n>        采集帧数，0 表示无限采集，默认：10
  -s, --stitch <mode>    拼接模式 (none/v/h)，默认：none
  -h, --help             显示帮助信息
```

### 宏配置

以下参数通过宏定义配置，可在编译时修改：

- `DEFAULT_BUF_COUNT`：缓冲区数量，默认为 3
- `DEFAULT_FORMAT`：默认像素格式，默认为 NV16
- `DEFAULT_DISPLAY`：是否启用显示输出，默认为 1（启用）

### 显示模式配置

以下三个宏必须且只能启用其中一个：

- `DE_SCALE_ENABLE`：DE 缩放模式（默认启用）
  - 在显示层进行等比例的缩放处理
  - 自动处理宽高比，当宽高比过大时拉伸成正方形
  - 适用于需要自适应显示的场景

- `DE_CROP_ENABLE`：DE 裁剪模式
  - 在显示层进行裁剪处理
  - 根据屏幕大小自动裁剪图像
  - 适用于图像尺寸大于屏幕尺寸的场景

- `DVP_CROP_ENABLE`：DVP 裁剪模式
  - 在 DVP 输出时进行裁剪
  - 居中对齐裁剪，保留图像中心区域
  - 适用于需要减少数据传输量的场景

### 使用示例

```bash
# 从通道 0 捕获 10 帧
test_vin -C 0 -c 10

# 从通道 1 连续捕获，使用 NV12 格式
test_vin -C 1 -f nv12 -c 0

# 垂直拼接模式（CH0 在上，CH1 在下）
test_vin -s v -c 10

# 水平拼接模式（CH0 在左，CH1 在右）
test_vin -s h -c 10

# 使用 CSI 设备
test_vin -d csi -C 0 -c 10
```

## 拼接模式

应用程序支持四种拼接模式：

1. **MPP_STITCH_INVALID**：无效模式，仅启用单个通道
   - 用于多通道采集时实际上只启用其中某一个通道的情况
   - 此时该通道独立工作，不涉及拼接逻辑

2. **MPP_STITCH_NONE**：不拼接模式
   - 各通道分别输出到各自的缓冲区队列
   - 多个通道可以同时工作，但输出互相独立

3. **MPP_STITCH_V_MODE**：垂直拼接模式
   - CH0 在上，CH1 在下
   - 两个通道的图像在垂直方向拼接

4. **MPP_STITCH_H_MODE**：水平拼接模式
   - CH0 在左，CH1 在右
   - 两个通道的图像在水平方向拼接

## 支持的格式

- **NV12**：YUV 4:2:0 格式
- **NV16**：YUV 4:2:2 格式
- **YUV400**：仅 Y 分量格式（灰度图）

## 扩展新设备

要添加对新视频输入设备的支持（例如 CSI）：

1. 在新文件中实现设备操作（例如 `test_vin_csi.c`）
2. 使用 `vin_dev_register()` 注册设备
3. 在 `test_vin_dev.h` 的 `enum vin_dev_type` 中添加新设备类型
