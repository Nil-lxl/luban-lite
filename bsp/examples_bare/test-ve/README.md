# 视频引擎测试 (test-ve)

测试视频引擎（VE）的图片解码功能。

## 调用关系

```
Bare Shell → test-ve → AIC VE 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_pic_dec.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_VE_TEST` | 启用视频引擎测试命令 | n | `LPKG_MPP` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用视频引擎测试命令
```

## 使用说明

### `test_pic_dec` — 图片解码测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<file>` | 图片文件路径，支持 `.jpg` 和 `.png` 格式 |

> 解码结果通过 framebuffer 输出到屏幕显示。

**使用示例：**

```bash
# 解码 JPEG 图片
test_pic_dec test.jpg

# 解码 PNG 图片
test_pic_dec test.png
```

## 依赖说明

- **上游依赖**：AIC VE 驱动
- **下游被依赖**：无
