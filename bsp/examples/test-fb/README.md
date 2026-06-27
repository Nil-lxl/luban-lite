# 帧缓冲测试 (test-fb)

## 一句话描述

Framebuffer 帧缓冲设备测试程序，验证显示输出与画面绘制功能。

## 调用关系

```
MSH Shell → test-fb → AIC Display/FB 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_fb.c` | 帧缓冲测试主程序 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_DISPLAY_TEST` | 启用 Display 驱动测试命令 | n | AIC_DISPLAY_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable Display driver test command
```

## 使用说明

### `test_fb` — 帧缓冲综合测试

支持查询/设置图层配置、Alpha 透明度、Color Key 以及色块显示和 vsync 测试。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-n, --get_layer_num` | 查询图层数量（视频层 + UI 层） |
| `-s, --get_screen_size` | 查询屏幕分辨率 |
| `-f, --get_fb_layer` | 查询 FB 层（Layer 0）配置 |
| `-l, --get_layer` | 查询所有图层（Layer 0 + Layer 1）配置 |
| `-L, --set_layer` | 设置图层配置，需配合 `-i -e/-d -w -h` |
| `-a, --get_alpha` | 查询 Alpha 透明度配置 |
| `-A, --set_alpha` | 设置 Alpha 透明度，需配合 `-e/-d -m -v` |
| `-k, --get_ck_cfg` | 查询 Color Key 配置 |
| `-K, --set_ck_cfg` | 设置 Color Key，需配合 `-e/-d -v` |
| `-e, --enable` | 启用标志 |
| `-d, --disable` | 禁用标志 |
| `-i, --id <n>` | 图层 ID [0, 1] |
| `-w, --width <n>` | 宽度 |
| `-h, --height <n>` | 高度 |
| `-m, --mode <n>` | Alpha 模式 [0: pixel, 1: global, 2: mix] |
| `-v, --value <n>` | 数值 [0, 255]，Color Key 时支持 0xRRGGBB 格式 |
| `-b, --colorblock` | 显示色块测试图（RGB 渐变 + 定位线） |
| `-r, --repeat` | 重复测试 vsync 等待时间 |
| `-u, --usage` | 显示帮助信息 |

**使用示例：**

```bash
# 查询图层数量
test_fb -n

# 查询屏幕分辨率
test_fb -s

# 查询所有图层配置
test_fb -l

# 设置 Layer 1 为启用，裁剪区域 800x480
test_fb -L -i 1 -e -w 800 -h 480

# 禁用 Layer 1
test_fb -L -i 1 -d -w 800 -h 480

# 设置 Alpha 为全局模式，值 128
test_fb -A -e -m 1 -v 128

# 禁用 Alpha
test_fb -A -d -m 0 -v 0

# 查询 Color Key 配置
test_fb -k

# 设置 Color Key 为启用，值 0x3F
test_fb -K -e -v 0x3F

# 禁用 Color Key
test_fb -K -d -v 0

# 显示色块测试图（验证 RGB 通道和定位）
test_fb -b

# 重复测试 vsync 等待时间（用于检测 DE 是否正常工作）
test_fb -r
```

## 依赖说明

- **上游依赖**：AIC Display 驱动
- **下游被依赖**：无
