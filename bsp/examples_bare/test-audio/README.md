# 音频测试 (test-audio)

测试音频播放和录音功能，支持 WAV 格式。

## 调用关系

```
Bare Shell → test-audio → AIC AUDIO 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `sound_arecord.c` | 测试实现 |
| `sound_play.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_AUDIO_DRV_TEST` | 启用音频测试命令 | n | `AIC_AUDIO_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用音频测试命令
```

## 使用说明

### `aplay` — 播放音频文件

**使用示例：**

```bash
# 播放 WAV 文件
aplay song.wav
```

### `arecord` — 录音并保存到文件

**使用示例：**

```bash
# 录音到 WAV 文件
arecord record.wav
```

## 依赖说明

- **上游依赖**：AIC AUDIO 驱动
- **下游被依赖**：无
