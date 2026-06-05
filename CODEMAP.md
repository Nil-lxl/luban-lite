# Luban-Lite 源码地图

> 本文件为 AI Agent 和开发者提供精确的源码路径索引。

## 应用层 (application/)

| 内核 | 应用 | 目录 | 说明 |
|------|------|------|------|
| Baremetal | Bootloader | `application/baremetal/bootloader/` | 裸机启动程序 |
| Baremetal | HelloWorld | `application/baremetal/helloworld/` | 裸机示例程序 |
| FreeRTOS | HelloWorld | `application/freertos/helloworld/` | FreeRTOS 示例程序 |
| RT-Thread | HelloWorld | `application/rt-thread/helloworld/` | RT-Thread 示例程序 |
| uCOS-II | HelloWorld | `application/ucos-ii/helloworld/` | uCOS-II 示例程序 |

## 板级配置 (target/)

### d11x

| 板卡 | 目录 |
|------|------|
| demo68-nopsram-xip | `target/d11x/demo68-nopsram-xip/` |

### d12p

| 板卡 | 目录 |
|------|------|
| demo88-nor | `target/d12p/demo88-nor/` |

### d12x

| 板卡 | 目录 |
|------|------|
| demo68-mmc | `target/d12x/demo68-mmc/` |
| demo68-nand | `target/d12x/demo68-nand/` |
| demo68-nor | `target/d12x/demo68-nor/` |
| hmi-nor | `target/d12x/hmi-nor/` |

### d13x

| 板卡 | 目录 |
|------|------|
| demo68-nor | `target/d13x/demo68-nor/` |
| demo88-nand | `target/d13x/demo88-nand/` |
| demo88-nor | `target/d13x/demo88-nor/` |
| demo88-nor-secure | `target/d13x/demo88-nor-secure/` |
| hspi100-nor | `target/d13x/hspi100-nor/` |
| kunlunpi88-nor | `target/d13x/kunlunpi88-nor/` |

### d21x

| 板卡 | 目录 |
|------|------|
| d215-demo88-nand | `target/d21x/d215-demo88-nand/` |
| d215-demo88-nor | `target/d21x/d215-demo88-nor/` |
| demo100-nand | `target/d21x/demo100-nand/` |
| demo100-nor | `target/d21x/demo100-nor/` |
| demo128-mmc | `target/d21x/demo128-mmc/` |
| demo128-nand | `target/d21x/demo128-nand/` |
| demo88-mmc | `target/d21x/demo88-mmc/` |

### g72x

| 板卡 | 目录 |
|------|------|
| demo48-nor | `target/g72x/demo48-nor/` |

### g73x

| 板卡 | 目录 |
|------|------|
| demo100-nor | `target/g73x/demo100-nor/` |
| demo68-nor | `target/g73x/demo68-nor/` |
| scan | `target/g73x/scan/` |


| 板卡 | 目录 |
|------|------|


| 板卡 | 目录 |
|------|------|


| 板卡 | 目录 |
|------|------|

## 测试示例 (bsp/examples/)

| 测试 | 目录 | 说明 |
|------|------|------|
| test-alarm | `bsp/examples/test-alarm/` | 闹钟测试 |
| test-audio | `bsp/examples/test-audio/` | 音频测试 |
| test-camera | `bsp/examples/test-camera/` | 摄像头测试 |
| test-can | `bsp/examples/test-can/` | CAN 测试 |
| test-canfd | `bsp/examples/test-canfd/` | CANFD 测试 |
| test-cap | `bsp/examples/test-cap/` | 输入捕获测试 |
| test-ce | `bsp/examples/test-ce/` | 加密引擎测试 |
| test-cir | `bsp/examples/test-cir/` | 红外接收测试 |
| test-clock | `bsp/examples/test-clock/` | 时钟测试 |
| test-crc32 | `bsp/examples/test-crc32/` | CRC32 校验测试 |
| test-csi | `bsp/examples/test-csi/` | CSI 接口测试 |
| test-ctp | `bsp/examples/test-ctp/` | 电容触摸屏测试 |
| test-dce | `bsp/examples/test-dce/` | DCE 测试 |
| test-dm-lib | `bsp/examples/test-dm-lib/` | 设备管理库测试 |
| test-dma | `bsp/examples/test-dma/` | DMA 测试 |
| test-dvp | `bsp/examples/test-dvp/` | DVP 测试 |
| test-fb | `bsp/examples/test-fb/` | 帧缓冲测试 |
| test-filesystem | `bsp/examples/test-filesystem/` | 文件系统测试 |
| test-gpai | `bsp/examples/test-gpai/` | GPAI (ADC) 测试 |
| test-gpio | `bsp/examples/test-gpio/` | GPIO 测试 |
| test-hrtimer | `bsp/examples/test-hrtimer/` | 高精度定时器测试 |
| test-i2c | `bsp/examples/test-i2c/` | I2C 测试 |
| test-inputcap | `bsp/examples/test-inputcap/` | 输入捕获测试 |
| test-keyadc | `bsp/examples/test-keyadc/` | 按键 ADC 测试 |
| test-lwip | `bsp/examples/test-lwip/` | 网络协议栈测试 |
| test-mbedtls | `bsp/examples/test-mbedtls/` | TLS 测试 |
| test-mbedtls-csrp | `bsp/examples/test-mbedtls-csrp/` | TLS CSRP 测试 |
| test-mmc | `bsp/examples/test-mmc/` | SD卡测试 |
| test-monkey | `bsp/examples/test-monkey/` | Monkey 测试 |
| test-mtd-file | `bsp/examples/test-mtd-file/` | MTD 文件测试 |
| test-mtop | `bsp/examples/test-mtop/` | MTOP 测试 |
| test-multipwm | `bsp/examples/test-multipwm/` | 多路 PWM 测试 |
| test-part | `bsp/examples/test-part/` | 分区表测试 |
| test-pbus | `bsp/examples/test-pbus/` | 外设总线测试 |
| test-pm | `bsp/examples/test-pm/` | 电源管理测试 |
| test-psadc | `bsp/examples/test-psadc/` | PSADC 测试 |
| test-qep | `bsp/examples/test-qep/` | QEP 测试 |
| test-qspi | `bsp/examples/test-qspi/` | QSPI 测试 |
| test-rtc | `bsp/examples/test-rtc/` | RTC 测试 |
| test-rtp | `bsp/examples/test-rtp/` | RTP 测试 |
| test-sdiomgr | `bsp/examples/test-sdiomgr/` | SDIO 管理器测试 |
| test-spinand | `bsp/examples/test-spinand/` | SPI NAND 测试 |
| test-spinor | `bsp/examples/test-spinor/` | SPI NOR 测试 |
| test-tsen | `bsp/examples/test-tsen/` | 温度传感器测试 |
| test-twinkle | `bsp/examples/test-twinkle/` | Twinkle 测试 |
| test-uart | `bsp/examples/test-uart/` | 串口测试 |
| test-userid | `bsp/examples/test-userid/` | User ID 测试 |
| test-vin | `bsp/examples/test-vin/` | 视频输入测试 |
| test-wdt | `bsp/examples/test-wdt/` | 看门狗测试 |
| test-wifi | `bsp/examples/test-wifi/` | WiFi 测试 |

## 裸机测试示例 (bsp/examples_bare/)

| 测试 | 目录 | 说明 |
|------|------|------|
| test-audio | `bsp/examples_bare/test-audio/` | 裸机音频测试 |
| test-can | `bsp/examples_bare/test-can/` | 裸机 CAN 测试 |
| test-ctp | `bsp/examples_bare/test-ctp/` | 裸机触摸屏测试 |
| test-dce | `bsp/examples_bare/test-dce/` | 裸机 DCE 测试 |
| test-efuse | `bsp/examples_bare/test-efuse/` | 裸机 eFuse 测试 |
| test-gpai | `bsp/examples_bare/test-gpai/` | 裸机 GPAI 测试 |
| test-gpio | `bsp/examples_bare/test-gpio/` | 裸机 GPIO 测试 |
| test-gpio-se | `bsp/examples_bare/test-gpio-se/` | 裸机 GPIO SE 测试 |
| test-hrtimer | `bsp/examples_bare/test-hrtimer/` | 裸机高精度定时器 |
| test-i2c | `bsp/examples_bare/test-i2c/` | 裸机 I2C 测试 |
| test-inputcap | `bsp/examples_bare/test-inputcap/` | 裸机输入捕获测试 |
| test-keyadc | `bsp/examples_bare/test-keyadc/` | 裸机按键 ADC 测试 |
| test-mmc | `bsp/examples_bare/test-mmc/` | 裸机 SD卡测试 |
| test-mtd | `bsp/examples_bare/test-mtd/` | 裸机 MTD 测试 |
| test-psadc | `bsp/examples_bare/test-psadc/` | 裸机 PSADC 测试 |
| test-pwm | `bsp/examples_bare/test-pwm/` | 裸机 PWM 测试 |
| test-rtc | `bsp/examples_bare/test-rtc/` | 裸机 RTC 测试 |
| test-rtp | `bsp/examples_bare/test-rtp/` | 裸机 RTP 测试 |
| test-spi | `bsp/examples_bare/test-spi/` | 裸机 SPI 测试 |
| test-spinand | `bsp/examples_bare/test-spinand/` | 裸机 SPI NAND 测试 |
| test-spinor | `bsp/examples_bare/test-spinor/` | 裸机 SPI NOR 测试 |
| test-tsen | `bsp/examples_bare/test-tsen/` | 裸机温度传感器测试 |
| test-userid | `bsp/examples_bare/test-userid/` | 裸机 User ID 测试 |
| test-ve | `bsp/examples_bare/test-ve/` | 裸机视频引擎测试 |
| test-xpwm | `bsp/examples_bare/test-xpwm/` | 裸机 XPWM 测试 |
| test-zlib | `bsp/examples_bare/test-zlib/` | 裸机 zlib 压缩测试 |

## 外设驱动 (bsp/peripheral/)

### Camera

| 芯片 | 源码目录 |
|------|----------|
| BF3A03 | `bsp/peripheral/camera/bf3a03/` |
| GC0308 | `bsp/peripheral/camera/gc0308/` |
| GC032A | `bsp/peripheral/camera/gc032a/` |
| GM7150 | `bsp/peripheral/camera/gm7150/` |
| N5 | `bsp/peripheral/camera/N5/` |
| NVP6158 | `bsp/peripheral/camera/nvp6158/` |
| OV2640 | `bsp/peripheral/camera/ov2640/` |
| OV2659 | `bsp/peripheral/camera/ov2659/` |
| OV5640 | `bsp/peripheral/camera/ov5640/` |
| OV7670 | `bsp/peripheral/camera/ov7670/` |
| OV9281 | `bsp/peripheral/camera/ov9281/` |
| SC030IOT | `bsp/peripheral/camera/sc030iot/` |
| SC031GS | `bsp/peripheral/camera/sc031gs/` |
| SC035 | `bsp/peripheral/camera/sc035/` |
| TP2825B | `bsp/peripheral/camera/tp2825b/` |
| TP9951 | `bsp/peripheral/camera/tp9951/` |
| XS9950 | `bsp/peripheral/camera/xs9950/` |

### 其他外设

| 类型 | 目录 | 说明 |
|------|------|------|
| BT | `bsp/peripheral/bt/` | 蓝牙模块 |
| Codec | `bsp/peripheral/codec/` | 音频编解码器 |
| Encoder | `bsp/peripheral/encoder/` | 编码器 |
| Gyro | `bsp/peripheral/gyro/` | 陀螺仪 |
| NFTL | `bsp/peripheral/nftl/` | NAND Flash 转换层 |
| RTC | `bsp/peripheral/rtc/` | 外部 RTC 芯片 |
| SPI LCD | `bsp/peripheral/spilcd/` | SPI LCD 屏幕 |
| SPI NAND | `bsp/peripheral/spinand/` | SPI NAND Flash 驱动 |
| SPI NAND Refresh | `bsp/peripheral/spinand_refresh/` | SPI NAND 刷新管理 |
| SPI NOR (SFUD) | `bsp/peripheral/spinor_sfud/` | SPI NOR Flash (SFUD 通用驱动) |
| Touch | `bsp/peripheral/touch/` | 触摸屏驱动 |
| Wireless | `bsp/peripheral/wireless/` | 无线模块 |

## AIC 组件 (packages/artinchip/)

| 组件 | 目录 | 说明 |
|------|------|------|
| AICP Dec | `packages/artinchip/aicp-dec/` | AICP 解码器 |
| Authorization | `packages/artinchip/aic-authorization/` | 授权管理 |
| Barcode | `packages/artinchip/barcode/` | 条码识别 |
| Burn-in | `packages/artinchip/burn-in/` | 老化测试 |
| Env | `packages/artinchip/env/` | 环境变量 |
| IP Manager | `packages/artinchip/ipmanager/` | IP 管理 |
| LVGL UI | `packages/artinchip/lvgl-ui/` | LVGL 图形界面 |
| Mini Boot | `packages/artinchip/mini_boot/` | 最小启动程序 |
| MPP | `packages/artinchip/mpp/` | 多媒体处理平台 |
| OF (设备树) | `packages/artinchip/of/` | 设备树解析 |
| OTA | `packages/artinchip/ota/` | 空中升级 |
| Pinmux Check | `packages/artinchip/pinmux-check/` | 引脚复用检查 |
| Profiler | `packages/artinchip/aic-profiler/` | 性能分析器 |
| Startup UI | `packages/artinchip/aic-startup-ui/` | 启动界面 |
| Thread Stack Trace | `packages/artinchip/thread_stack_trace/` | 线程栈跟踪 |
| UDS | `packages/artinchip/uds/` | UDS 诊断 |
| User ID | `packages/artinchip/userid/` | 用户 ID |
| XIP RAM Code | `packages/artinchip/xip_ramcode/` | XIP RAM 代码 |
| Zip FS | `packages/artinchip/zipfs/` | ZIP 文件系统 |

## 第三方组件 (packages/third-party/)

| 组件 | 目录 | 说明 |
|------|------|------|
| ADBD | `packages/third-party/adbd/` | Android Debug Bridge 守护进程 |
| AT24Cxx | `packages/third-party/at24cxx/` | AT24Cxx EEPROM 驱动 |
| Beep | `packages/third-party/beep/` | 蜂鸣器驱动 |
| Benchmark | `packages/third-party/benchmark/` | 性能基准测试 |
| CherryUSB (v1.0.0) | `packages/third-party/cherryusb-1.0.0/` | USB Host/Device 协议栈 (旧版) |
| CherryUSB (v1.5.0) | `packages/third-party/cherryusb/` | USB Host/Device 协议栈 |
| cJSON | `packages/third-party/cJSON-1.7.16/` | JSON 解析库 |
| CPU Load | `packages/third-party/cpu_load/` | CPU 负载监控 |
| CPU Usage | `packages/third-party/cpu_usage/` | CPU 使用率统计 |
| DFS | `packages/third-party/dfs/` | 设备文件系统 (裸机模式) |
| FDTLib | `packages/third-party/fdtlib/` | Flattened Device Tree 库 |
| FFmpeg | `packages/third-party/ffmpeg/` | 多媒体处理框架 |
| FreeRTOS Wrapper | `packages/third-party/FreeRTOS-Wrapper/` | FreeRTOS API 兼容层 (RT-Thread) |
| FreeType | `packages/third-party/freetype/` | 字体渲染库 |
| GIF | `packages/third-party/gif/` | GIF 动画解码 |
| i2c-tools | `packages/third-party/i2c-tools/` | I2C 调试工具 |
| LevelX | `packages/third-party/levelx/` | 高性能嵌入式文件层 |
| libFLAC | `packages/third-party/libFLAC/` | FLAC 无损音频编解码库 |
| libmodbus | `packages/third-party/libmodbus/` | Modbus 协议库 |
| libopus | `packages/third-party/libopus/` | Opus 音频编解码库 |
| librws | `packages/third-party/librws/` | WebSocket 客户端库 |
| LittleFS | `packages/third-party/littlefs/` | 高完整性嵌入式文件系统 |
| LLM Chat | `packages/third-party/llm_chat/` | 大语言模型聊天 |
| LwIP | `packages/third-party/lwip/` | 轻量级 TCP/IP 协议栈 |
| mbedTLS | `packages/third-party/mbedtls/` | SSL/TLS 加密库 |
| MemLeak | `packages/third-party/memleak/` | 内存泄漏检测 |
| MHz | `packages/third-party/mhz/` | CPU 频率测量 |
| MicroPython | `packages/third-party/micropython-1.13.0/` | MicroPython 脚本引擎 |
| minimp3 | `packages/third-party/minimp3/` | 轻量级 MP3 解码库 |
| miniz | `packages/third-party/miniz/` | 数据压缩库 |
| mklittlefs | `packages/third-party/mklittlefs/` | LittleFS 镜像制作工具 |
| Netutils | `packages/third-party/netutils/` | 网络工具集 |
| NimBLE | `packages/third-party/nimble/` | 开源蓝牙 5.0 协议栈 |
| OTA Downloader | `packages/third-party/ota_downloader/` | HTTP OTA 固件下载器 |
| Paho MQTT | `packages/third-party/pahomqtt/` | Eclipse Paho MQTT 客户端 |
| protobuf-c | `packages/third-party/protobuf-c/` | Protocol Buffers C 序列化库 |
| PTPd | `packages/third-party/ptpd/` | IEEE 1588 精确时钟同步 |
| RTT Auto Exe Cmd | `packages/third-party/rtt_auto_exe_cmd/` | RT-Thread 自动执行命令 |
| SQLite | `packages/third-party/sqlite/` | 嵌入式 SQL 数据库引擎 |
| WebClient | `packages/third-party/webclient/` | HTTP/HTTPS 客户端 |
| Zephyr Bluetooth | `packages/third-party/zephyr-bluetooth/` | Zephyr 蓝牙协议栈 |
| zlib | `packages/third-party/zlib/` | Zlib 数据压缩库 |

## MPP 框架

| 模块 | 目录 | 说明 |
|------|------|------|
| Base | `packages/artinchip/mpp/base/` | 基础设施 |
| FB | `packages/artinchip/mpp/fb/` | 帧缓冲 |
| GE | `packages/artinchip/mpp/ge/` | 图形引擎 |
| Player | `packages/artinchip/mpp/middle_media/player/` | 播放器 |
| VE | `packages/artinchip/mpp/ve/` | 视频引擎 |
| VIN | `packages/artinchip/mpp/vin/` | 视频输入 |


## 构建系统

| 文件 | 说明 |
|------|------|
| `SConstruct` | SCons 入口脚本 |
| `SConscript` | 根目录编译脚本 |
| `Kconfig` | 根配置文件 |
| `tools/scripts/aic_build.py` | 构建核心逻辑 |
| `tools/scripts/mk_image.py` | 镜像生成 |
| `tools/scripts/gen_partition_table.py` | 分区表生成 |

## 公共头文件

| 头文件 | 说明 |
|--------|------|
| `aic_core.h` | 核心类型定义 (u32/s32 等) |
| `aic_common.h` | 公共定义 |
| `rtconfig.h` | RT-Thread 配置 |
| `cconfig.h` | 组件配置 |

## DRV 层

| 模块 | 源码目录 | 头文件 |
|------|----------|--------|
| ADCIM | `bsp/artinchip/drv/adcim/` | — |
| Audio | `bsp/artinchip/drv/audio/` | — |
| CAN | `bsp/artinchip/drv/can/` | — |
| CANFD | `bsp/artinchip/drv/canfd/` | — |
| Cap  | `bsp/artinchip/drv/cap/` | — |
| CE | `bsp/artinchip/drv/ce/` | — |
| CIR | `bsp/artinchip/drv/cir/` | `bsp/artinchip/include/drv/drv_cir.h` |
| DCE | `bsp/artinchip/drv/dce/` | — |
| Display | `bsp/artinchip/drv/display/` | `bsp/artinchip/include/drv/aic_drv_de.h` |
| DMA | `bsp/artinchip/drv/dma/` | `bsp/artinchip/include/drv/drv_dma.h` |
| DVP | `bsp/artinchip/drv/dvp/` | — |
| EFUSE | `bsp/artinchip/drv/efuse/` | `bsp/artinchip/include/drv/drv_efuse.h` |
| EPWM | `bsp/artinchip/drv/epwm/` | `bsp/artinchip/include/drv/drv_epwm.h` |
| GE | `bsp/artinchip/drv/ge/` | `bsp/artinchip/include/drv/aic_drv_ge.h` |
| GPAI | `bsp/artinchip/drv/gpai/` | — |
| GPIO | `bsp/artinchip/drv/gpio/` | `bsp/artinchip/include/drv/aic_drv_gpio.h` |
| HRTimer | `bsp/artinchip/drv/hrtimer/` | `bsp/artinchip/include/drv/drv_hrtimer.h` |
| I2C | `bsp/artinchip/drv/i2c/` | — |
| I2S | `bsp/artinchip/drv/i2s/` | — |
| InputCap | `bsp/artinchip/drv/inputcap/` | — |
| MAC | `bsp/artinchip/drv/mac/` | — |
| MTOP | `bsp/artinchip/drv/mtop/` | `bsp/artinchip/include/drv/aic_drv_mtop.h` |
| PM | `bsp/artinchip/drv/pm/` | `bsp/artinchip/include/drv/pm_cfg.h` |
| PSADC | `bsp/artinchip/drv/psadc/` | `bsp/artinchip/include/drv/drv_psadc.h` |
| PWM | `bsp/artinchip/drv/pwm/` | — |
| QEP | `bsp/artinchip/drv/qep/` | — |
| QSPI | `bsp/artinchip/drv/qspi/` | `bsp/artinchip/include/drv/drv_qspi.h` |
| RTC | `bsp/artinchip/drv/rtc/` | — |
| RTP | `bsp/artinchip/drv/rtp/` | — |
| SDMC | `bsp/artinchip/drv/sdmc/` | — |
| SPI | `bsp/artinchip/drv/spi/` | — |
| SPIENC | `bsp/artinchip/drv/spienc/` | `bsp/artinchip/include/drv/drv_spienc.h` |
| SPINAND | `bsp/artinchip/drv/spinand/` | — |
| SPINOR | `bsp/artinchip/drv/spinor/` | — |
| SYSCFG | `bsp/artinchip/drv/syscfg/` | — |
| TSEN | `bsp/artinchip/drv/tsen/` | — |
| UART | `bsp/artinchip/drv/uart/` | `bsp/artinchip/include/drv/aic_drv_uart.h` |
| VE | `bsp/artinchip/drv/ve/` | `bsp/artinchip/include/drv/aic_drv_ve.h` |
| WDT | `bsp/artinchip/drv/wdt/` | `bsp/artinchip/include/drv/aic_drv_wdt.h` |
| WRI | `bsp/artinchip/drv/wri/` | — |
| XPWM | `bsp/artinchip/drv/xpwm/` | — |

## HAL 层

| 模块 | 源码目录 | 头文件 |
|------|----------|--------|
| ADCIM | `bsp/artinchip/hal/adcim/` | `bsp/artinchip/include/hal/hal_adcim.h` |
| Audio | `bsp/artinchip/hal/audio/` | `bsp/artinchip/include/hal/hal_audio.h` |
| AXICFG | `bsp/artinchip/hal/axicfg/` | `bsp/artinchip/include/hal/hal_axicfg.h` |
| CAN | `bsp/artinchip/hal/can/` | `bsp/artinchip/include/hal/hal_canfd.h` |
| CANFD | `bsp/artinchip/hal/canfd/` | `bsp/artinchip/include/hal/hal_canfd.h` |
| CAP | `bsp/artinchip/hal/inputcap/` | `bsp/artinchip/include/hal/hal_inputcap.h` |
| CE | `bsp/artinchip/hal/ce/` | `bsp/artinchip/include/hal/hal_ce.h` |
| CIR | `bsp/artinchip/hal/cir/` | `bsp/artinchip/include/hal/hal_cir.h` |
| CMU | `bsp/artinchip/hal/cmu/` | `bsp/artinchip/include/hal/aic_hal_clk_cmu.h` |
| DCE | `bsp/artinchip/hal/dce/` | `bsp/artinchip/include/hal/hal_dce.h` |
| Display | `bsp/artinchip/hal/display/` | `bsp/artinchip/include/hal/aic_hal_de.h` |
| DMA | `bsp/artinchip/hal/dma/` | `bsp/artinchip/include/hal/hal_dma.h` |
| DVP | `bsp/artinchip/hal/dvp/` | `bsp/artinchip/include/hal/hal_dvp.h` |
| EFuse | `bsp/artinchip/hal/efuse/` | `bsp/artinchip/include/hal/hal_efuse.h` |
| GE | `bsp/artinchip/hal/ge/` | `bsp/artinchip/include/hal/aic_hal_ge.h` |
| GPAI (ADC) | `bsp/artinchip/hal/gpai/` | `bsp/artinchip/include/hal/hal_gpai.h` |
| GPIO | `bsp/artinchip/hal/gpio/` | `bsp/artinchip/include/hal/aic_hal_gpio.h` |
| I2C | `bsp/artinchip/hal/i2c/` | `bsp/artinchip/include/hal/hal_i2c.h` |
| I2S | `bsp/artinchip/hal/i2s/` | `bsp/artinchip/include/hal/hal_i2s.h` |
| MAC | `bsp/artinchip/hal/mac/` | `bsp/artinchip/include/hal/hal_mac.h` |
| MTOP | `bsp/artinchip/hal/mtop/` | `bsp/artinchip/include/hal/aic_hal_mtop.h` |
| PBus | `bsp/artinchip/hal/pbus/` | `bsp/artinchip/include/hal/hal_pbus.h` |
| PSADC | `bsp/artinchip/hal/psadc/` | `bsp/artinchip/include/hal/hal_psadc.h` |
| PWM | `bsp/artinchip/hal/pwm/` | `bsp/artinchip/include/hal/hal_pwm.h` |
| PWMCS | `bsp/artinchip/hal/pwmcs/` | — |
| QSPI | `bsp/artinchip/hal/qspi/` | `bsp/artinchip/include/hal/hal_qspi.h` |
| RTC | `bsp/artinchip/hal/rtc/` | `bsp/artinchip/include/hal/hal_rtc.h` |
| RTP | `bsp/artinchip/hal/rtp/` | `bsp/artinchip/include/hal/hal_rtp.h` |
| SDMC (SD卡) | `bsp/artinchip/hal/sdmc/` | `bsp/artinchip/include/hal/hal_sdmc.h` |
| SPIENC | `bsp/artinchip/hal/spienc/` | `bsp/artinchip/include/hal/hal_spienc.h` |
| SYSCFG | `bsp/artinchip/hal/syscfg/` | `bsp/artinchip/include/hal/hal_syscfg.h` |
| TSEN  | `bsp/artinchip/hal/tsen/` | `bsp/artinchip/include/hal/hal_tsen.h` |
| UART | `bsp/artinchip/hal/uart/` | `bsp/artinchip/include/hal/aic_hal_uart.h` |
| VE  | `bsp/artinchip/hal/ve/` | `bsp/artinchip/include/hal/aic_hal_ve.h` |
| Watchdog | `bsp/artinchip/hal/wdt/` | `bsp/artinchip/include/hal/hal_wdt.h` |
| WRI | `bsp/artinchip/hal/wri/` | `bsp/artinchip/include/hal/hal_wri.h` |
| XPWM | `bsp/artinchip/hal/xpwm/` | `bsp/artinchip/include/hal/hal_xpwm.h` |
| XSPI | `bsp/artinchip/hal/xspi/` | `bsp/artinchip/include/hal/hal_xspi.h` |

## SoC 平台层

| SoC | 启动代码 | 时钟配置 | 中断路由 | Kconfig |
|-----|----------|----------|----------|---------|
| d11x | `bsp/artinchip/sys/d11x/startup_gcc.S` | `bsp/artinchip/sys/d11x/aic_hal_clk.c` | `bsp/artinchip/sys/d11x/isr.c` | `bsp/artinchip/sys/d11x/Kconfig.chip` |
| d12p | `bsp/artinchip/sys/d12p/startup_gcc.S` | `bsp/artinchip/sys/d12p/aic_hal_clk.c` | `bsp/artinchip/sys/d12p/isr.c` | `bsp/artinchip/sys/d12p/Kconfig.chip` |
| d12x | `bsp/artinchip/sys/d12x/startup_gcc.S` | `bsp/artinchip/sys/d12x/aic_hal_clk.c` | `bsp/artinchip/sys/d12x/isr.c` | `bsp/artinchip/sys/d12x/Kconfig.chip` |
| d13x | `bsp/artinchip/sys/d13x/startup_gcc.S` | `bsp/artinchip/sys/d13x/aic_hal_clk.c` | `bsp/artinchip/sys/d13x/isr.c` | `bsp/artinchip/sys/d13x/Kconfig.chip` |
| d21x | `bsp/artinchip/sys/d21x/startup_gcc.S` | `bsp/artinchip/sys/d21x/aic_hal_clk.c` | `bsp/artinchip/sys/d21x/isr.c` | `bsp/artinchip/sys/d21x/Kconfig.chip` |
| g72x | `bsp/artinchip/sys/g72x/startup_gcc.S` | `bsp/artinchip/sys/g72x/aic_hal_clk.c` | `bsp/artinchip/sys/g72x/isr.c` | `bsp/artinchip/sys/g72x/Kconfig.chip` |
| g73x | `bsp/artinchip/sys/g73x/startup_gcc.S` | `bsp/artinchip/sys/g73x/aic_hal_clk.c` | `bsp/artinchip/sys/g73x/isr.c` | `bsp/artinchip/sys/g73x/Kconfig.chip` |

## OSAL (OS 抽象层)

| 文件 | 说明 |
|------|------|
| `kernel/common/include/osal/aic_osal.h` | OSAL 统一入口 |
| `kernel/common/include/osal/aic_osal_baremetal.h` | 裸机适配 |
| `kernel/common/include/osal/aic_osal_freertos.h` | FreeRTOS 适配 |
| `kernel/common/include/osal/aic_osal_rtthread.h` | RT-Thread 适配 |
| `kernel/common/include/osal/aic_osal_ucos_ii.h` | uCOS-II 适配 |

## 内核

| 内核 | 源码目录 | 配置宏 |
|------|----------|--------|
| Baremetal | `kernel/baremetal/` | `KERNEL_BAREMETAL` |
| FreeRTOS | `kernel/freertos/` | `KERNEL_FREERTOS` |
| RT-Thread | `kernel/rt-thread/` | `KERNEL_RTTHREAD` |
| uCOS-II | `kernel/ucos-ii/` | `KERNEL_UCOS_II` |
