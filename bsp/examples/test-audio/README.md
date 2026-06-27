# 音频测试 (test-audio)

## 一句话描述

音频设备综合测试程序集，支持 WAV 文件播放/录音、I2S 回环测试和双工录放。

## 调用关系

```
MSH Shell → test-audio → AIC Audio/I2S 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `aplay.c` | WAV 文件播放，支持指定音频设备 |
| `arecord.c` | WAV 文件录音，支持指定录音设备 |
| `aduplex.c` | I2S 双工：同时录音和播放 |
| `test_audio_loopback.c` | I2S 音频流回环测试 |
| `test_i2s_loopback.c` | I2S 硬件回环测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_AUDIO_DRV_TEST` | 启用音频驱动测试命令 | n | AIC_AUDIO_DRV |
| `AIC_I2S_DRV_TEST` | 启用 I2S 驱动测试命令 | n | AIC_I2S_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable Audio driver test command / Enable I2S driver test command
```

## 使用说明

### `aplay` — 播放 WAV 文件

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<file>` | WAV 文件路径 |
| `[<device>]` | 可选，音频输出设备名称 |

**使用示例：**

```bash
# 播放 SD 卡上的 WAV 文件
aplay /sdcard/test.wav

# 指定音频设备 snd0 播放
aplay /sdcard/test.wav snd0

```

### `arecord` — 录制 WAV 文件

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<file>` | 输出 WAV 文件路径 |
| `[<device>]` | 可选，录音设备名称 |

**使用示例：**

```bash
# 录音到 SD 卡
arecord /sdcard/rec.wav

```

### `i2s_record_play` — I2S 双工录放

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<record_file>` | 录音输出路径 |
| `<play_file>` | 播放文件路径 |

**使用示例：**

```bash
# 同时录音和播放
i2s_record_play /sdcard/rec.wav /sdcard/play.wav

```

### `test_audio_loopback` — I2S 音频流回环测试

**使用示例：**

```bash
# 执行 I2S 音频流回环测试
test_audio_loopback

```

### `test_i2s_loopback` — I2S 硬件回环测试

**使用示例：**

```bash
# 执行 I2S 硬件回环测试
test_i2s_loopback

```

## 依赖说明

- **上游依赖**：AIC Audio 驱动, AIC I2S 驱动, POSIX 文件系统
- **下游被依赖**：无
