# V1.1.2 #
## 新增 ##
- USB：
  - HID keyboard：支持命令行构造字符输入
  - MSC Host：支持无分区表的设备挂载
  - 集成了MacOS驱动试用版
  - 支持用VE完成JPEG旋转
- LVGL：
  - 适配AiUIBuilder
  - Demo中增加Key adc按钮示例
- MPP：
  - 新增WMA、FLAC音频封装格式
  - 支持AAC解码（因开源协议问题，公版不支持AAC）
  - 支持MKV、TS、FLV视频封装格式
- 烧写：支持HID模式的烧写；优化UART烧写的兼容性
- Application：支持自动install资源文件的目录
- D21x/D13x/D12x：支持栈回溯功能
- SFUD：增加安全寄存器的读写接口
- OTA：增加ymodel协议支持
- PWM：支持动态调整clk频率
- I2S：增加数据的直传接口
- QEP：增加D13x平台的支持
- RTP：支持RTP IO当作普通ADC来使用
- OneStep：支持自动打开、保存所有defconfig文件
- BT8858：新增HID device支持，并适配iOS和Android
- TP2825：支持自动检测隔行模式
- 新增方案配置：D13x hspi（衡山PI）
- 新增器件支持：
  - WiFi：AIC8800
  - NAND：XT26G04C、FM25S01BI3
  - Touch：ili2511、ft6336、c123hax022
  - Panel：ili9327
  - Audio codec：cs4344
  - RTC：pcf8563
## 优化 ##
- USB副屏：
  - Display：动态调整USB传输的报文长度
  - OSD：增加背光控制滑动条；支持保存用户的配置参数
  - UDC：优化setup报文的处理
  - 优化复合设备的加载处理
- G73：扫码Demo完成多处性能和稳定性优化；支持画面旋转
- MPP：增加独立的Audio render框架，优化Audio播放相关的功能
- Display：优化Vsync的处理逻辑
- CMU：优化PLL的展频处理
- littlefs：优化读写性能
- UMM：优化初始化的耗时
- SFUD：优化数据的等待延迟
- RTP：优化休眠唤醒后的中断处理
- NAND：优化OOB的访问处理；优化block层的访问性能
- PBP：D13x/G73：优化FatFS对不同cluster size的兼容性
- UART：优化FIFO的触发水位配置；优化DMA模式的处理逻辑
- syscfg：重构代码，更容易支持多平台
- UART烧写：大幅优化传输速度
- FreeType：优化代码体积
- RTP：优化自动校准的方法
- Audio codec：es8388优化启动时间
- SPI：调整Bit模式的分频计算，让频率更精准

## 修改 ##
- LVGL：
  - 修正自动删除Screen时的逻辑漏洞
  - 所有Demo修改资源文件为install方式
  - 打包FatFS镜像：改用整个分区size，不再自动计算
- GE：针对D13x平台增加格式有效性检查
- UseID：修正一处buf的使用
- RTP：去掉对 x_plate和y_plate 参数的取值范围限制
- CE：Hash：修正多次计算的数据处理
- aic-authorization：修正数据buf的cache处理
- OTA：修正版本回滚的处理
- LwIP：修正IP4中next-hop的处理
- workqueue：修正接收和发送的处理流程
- 启动：统一使用ArtInChip启动画面
- VSCode：修正load方式的参数配置
- 栈回溯：默认关闭此功能，可在menuconfig中打开
- OneStep：解决Luban和Luban-Lite之间切换后的命令残留问题
- Version：统一RT-Thread、FreeRTOS、Baremetal系统的version命令log内容
- CherryUSB：修正裸机模式下的互斥锁机制
- 打包镜像：当资源文件路径无效的时候，生成一个空的文件镜像
- 烧写：修正sparse格式的烧写失败问题；G73减少Boot的代码段空间
- UART：完善可选的波特率配置表
- 开机动画：改善竖屏的兼容处理
- syscfg：修正D12x获取LDO的返回值
- BootLoader：不支持用户手动选择elf区域配置
- 清理开源协议的规范使用问题



# V1.1.1 #
## 新增 ##
- FreeType：D13x支持独立使用PSRAM Heap
- MPP：支持FLAC的Audio封装格式
- USB Display：增加Linux Host端的驱动安装包
- Camera：框架增加更多的摄像头调节接口；GM7150支持调整色度
- 打包：支持自动计算NFTL分区的镜像大小
- Benchmark：支持单精度浮点的模式
- 新增示例：test_camera
- I2C：新增软件模拟I2C的驱动实现
- G73x：支持栈回溯功能
- Touch：框架中支持翻转、旋转、缩放、裁剪，目前已适配GT911
- 新增方案：G73x scan
- 新增器件：
- NAND：XT26G02D

## 优化 ##
- Boot：自动计算BootLoader的size信息，默认将其放在PSRAM的末尾
- MPP：优化第一次播放时的噪声处理
- GMAC：优化内存资源的占用
- USB Display：优化Touch的同步处理；OSD对屏幕分辨率自适应；支持屏幕有效区域的裁剪
- WiFi：RTL完善功能支持
- test_dvp & UVC：优化调试log中的信息描述，提升易读性
- 启动动画：优化内存资源占用；支持 480x272 小分辨率的屏幕
- LVGL V9：优化多处资源管理和处理性能；支持图片cache的大小配置

## 修改 ##
- LVGL V9：默认关闭性能监测；支持导出Framebuffer；完善绘画Buf的处理；支持从SD卡中读取RTP配置
- AWTK：修正第一帧的vsync处理
- DE：修正CCM、GAMMA参数
- Audio：调整AMIC的默认配置参数
- 启动动画：修正动画过程中的图片切换同步
- LittleFS、PSADC：修正打开 RT_DEBUG 后的编译问题
- 修正colorblock在屏幕裁剪模式中的越界问题；
- 修正同时播放视频时的画面撕裂问题

# V1.1.0 #
## 新增 ##
- 支持LVGL9，并适配了所有Demo
- 支持Win7系统环境中编译
- 支持UCOS-II内核
- 支持在SDK之外的目录生成FS镜像
- 增加强制升级模式
- SPIENC：支持D13x的加密启动
- 新增SoC型号：G730BDU
- 新增方案支持：G73 demo68-nor、D215 demo88-nand/nor
- 新增器件支持：
- 屏幕：LCOS_HX7033
- Touch：c145hax01、cst3240多点触摸
- Camera：GC032A、GC0308、SC031IOT
- BT：bt8852a
- 新增第三方包：protobuf-c
-test_efuse：增加eFuse烧写示例


## 优化 ##
- USB：
- Touch功能适配Win11系统
- 支持UAC和MPP之间动态切换
- ACM支持模拟多路串口
- USB副屏：
  - 支持本地视频播放，并支持简单的播放控制
  - 支持动态的开、关USB显示
  - 支持动态的开、关UAC播放
  - 完成多处兼容性改进
  - 性能优化，包括USB零拷贝、减少旋转Buf
- I2C：优化频率设置，支持更宽的频率范围
- BootLoader：调整链接地址到尾部
- NAND：支持变长的器件ID；完善512MB容量的处理
- SPI：支持动态申请、释放DMA通道
- ymodem：支持指定路径的文件传输
- WRI：简化WRI和RTC模块的依赖关系
- pinmux：重构pinmux.c，支持唤醒按键配置；并提供OneStep命令update来升级客户扩展的board代码
- aicupg：增加数据的CRC校验；烧写进度条可配置
- Camera：在menuconfig中的配置移至'Driver->peripheral'

## 修改 ##
- FAL：完善数据Buf的限制条件
- Touch：修正cst3240 的数据异常
- USB：UVC device修正Buf的申请个数限制
- MPP：修正Jpeg缩放的size处理；完善PNG解码的错误处理；支持外部指定VE的输入Buf
- Audio：修正AMIC的处理流程
- Boot：
  - 启动过程中会检查OS的size有效性
  - 修正启动动画只有一幅图的处理
- 可以通过menuconfig配置启动动画的坐标
- AWTK：修正部分场景的内存分配和使用
- Eclipse：修正MPP模块的编译脚本处理
- CIR：修正NEC、RC5部分情况下的解码错误
- FatFS：支持中文字符
- RTT kernel：修正打开 RT_DEBUG 后的编译错误
- test_wdt：创建单独线程来完成喂狗动作
- OneStep：在Windows系统中列出所有defconfig（含BootLoader）


# V1.0.6 #
## 新增 ##
- 新增型号 D215，支持USB扩展屏功能
- LVGL：
  - 新增LVGL官方的music和benchmark demo
  - 新增电梯、咖啡机、蒸箱、86盒 和 USB扩展屏OSD 的demo
  - 支持从内存解码JPG/PNG图片到控件
- MPP：支持从DVP采集数据进行JPG编码，并存储到文件，录像文件支持MP4封装。
- PSRAM：支持封装型号D122BCV1、D122BCV2、TR230、JYX58、DR128、JYX68
- USB：支持WinUSB 1.0；支持UVC & UAC device；
- PM：支持设置唤醒源；支持light sleep mode
- SDMC：支持DDR模式
- WDT：增加寄存器的写保护
- 存储：支持NOR + NAND/NOR方案；NAND的rodata分区也增加坏块管理
- 启动：增加启动动画
- 安全：支持镜像文件中的SPL数据加密
- OneStep：新增 km/bm/ma/mu 命令
- 系统：增加waitqueue API
- 新增器件支持：
  - WiFi：ASR5505
  - 屏幕：axs15231b、nt35560、ST77922
  - DVP CVBS In：GM7150、TP2825/TB9950
  - Touch：cst3240、tw31xx、cst826、gsl3676、st77922
- 新增软件包：aic-authorization、ipmanager
- 新增第三方包：MicroPython
- 支持烤机测试
- test_dvp：支持使用GE进行旋转后再显示
- 增加示例：LwIP HTTP server test、test_gpio_key
- 增加方案配置：D215 demo88 nand/nor

## 优化 ##
- MPP：
  - Player：优化性能，包括解码流程、快速seek处理、内存管理流程等
  - Audio：优化重新播放时的延迟
- DE：优化Scaler效果
- Audio Codec：优化TLV320的播放和录音增益
- OneStep：简化BootLoader的编译方法（lunch清单中隐藏Bootloader配置，不再需要预编译bootloader.bin）
- 许可协议统一使用Apache-2.0
- 系统：
  - 优化memcpy的性能
  - 优化刷Cache的性能
  - package/artinchip中的软件包支持调整优先加载
- USB：
  - MSC支持热插拔，并优化传输的稳定性
  - 优化接入Hub时的处理，支持Hub级联
  - HID设备支持多个report ID
- Touch设备支持五点触摸
- 支持动态的注册模式
- CAP：提升频率的计算精度
- Touch：优化外设驱动的编译管理，简化用户的配置

## 修改 ##
- MPP：改进D21x的PNG解码兼容性
- USB：修正USB1、Hub的数据处理逻辑
- FatFS：分区时给文件夹预留足够的空间
- OTA：全平台打开OTA配置
- BootLoader：D13x中BootLoader默认运行在PSRAM中
- Eclipse：解决Freetype的编译问题
- 系统：
  - Panic时可以打印出完整的寄存器信息
  - 修正负数、浮点数的打印处理
  - 修正部分情况下的栈对齐问题
  - 完善get_tick接口的进位处理
- aicupg：
  - 当检测到SD卡/U盘拔出时重启系统
  - 改善和Win7系统连接的稳定性
- UART：为Rx IO配置上拉电阻
- test_uart：修正接收过长数据时的循环计数溢出问题；流控测试中支持设置波特率；修正线程退出流程
- test_psadc：从syscfg中读取电压参考值


# V1.0.5 #
## 新增 ##
- 调屏：支持和 AiPQ V1.1.1 工具配合使用
- 新增LVGL Demo：
- 支持VSCode 模拟器的工程导入
- 增加压力测试、独立控件、Gif、slide、lv_ffmpeg、DVP回显等参考实现
- MPP Player：支持avi封装格式
- USB Device：增加MTP支持
- USB Host：支持GPT分区格式
- PM：支持休眠时进入DDR自刷新
- aicupg：
  -支持和AiBurn工具配合完成Flash擦写功能
  -支持通过USB/UART获取设备侧的运行log
  -支持用命令进入U-Boot 升级模式
  -烧写时的进度条界面支持90/270度的旋转
- OTA：支持NFTL格式的Data分区升级；支持NOR、MMC介质
- TSen：增加对不同CP测试数据的兼容
- UART：增加流控功能支持；增加时钟分频的最佳匹配算法；增加软件模拟485的模式
- FS：支持生成 stripped FatFS 镜像，减小镜像文件大小
- DFS：SD0设备节点支持GPT分区格式
- GMAC：支持基于CHIP ID生成MAC地址；增加IPv6支持
- I2C：增加中断模式的处理流程；增加slave模式支持
- 新增SPI NAND：F35SQB004G、GD5F1GM7xUExxG、ZB35Q04A
- 新增屏幕：
  - RGB：gc9a01a
  - LVDS：LT8911
  -MIPI DSI：hx8394/ili9881c/st7703
  - i8080：st7789
- 新增CTP：st16xx、axs15260
- 新增第三方包：cJSON-1.7.16、mbedtls
- 新增aicp图片压缩库，压缩率更高
- 新增示例：test-twinkle、spi_flash、test_tsen_htp
- 新增工具支持：elf的size分析、编译时自动检测pinmux冲突、镜像文件大小匹配检查、version命令显示SoC型号
- OneStep：新增list-module命令，可查看当前方案中已打开的模块信息

## 优化 ##
- ADC：优化校准参数的算法
- Audio：优化关闭播放时的噪声；减少处理延迟
- DE：LVDS的双link可以单独配置参数
- PSRAM：优化初始化参数，提升稳定性
- USB：优化ADB写FatFS分区的性能
- SPI NAND：优化Page buf的内存拷贝流程；减少数据传输中的delay；
- SPI：优化DMA传输的结束状态判断
- SFUD：优化参数配置，提升工作频率
- D21x：优化DDR参数提升稳定性和兼容性
- aicupg：提升U盘烧录的兼容性

## 修改 ##
- MPP player：修正多个兼容性问题
- OTA：修正HTTP中的一处内存泄漏
- D12x：修改LDO1x电压为1.2V，稳定性更好
- D12x demo68：显示格式改为RGB565，和LVGL匹配
- aicupg：修正4K page size的处理
- GPAI：周期模式的默认周期改为 18ms
- UART：修正485模式的数据传输错误




# V1.0.4 #
## 新增 ##
- MPP：支持WAV文件解析；支持超短音频的播放
- Display：支持AiPQ调屏工具
- USB：支持MIDI、USB Audio、Device MSC、CDC
- IRQ：D12x、D13x支持中断嵌套、超级中断机制
- UART：支持 485 软件模式；支持DMA方式；支持流控功能
- LwIP：增加网络配置工具；增加PTPD协议支持；支持DHCP server；增加回环测试命令
- 新增PSADC、QEP驱动
- 烧写过程增加图形的进度条界面，进度条效果可配置
- aicupg：支持通过SD卡升级指定分区
- PBP：支持关闭PBP启动过程中的log
- 支持使用PWM实现hrtimer
- D12x支持PM流程
- 增加命令"scons --list-mem"，方便查看当前方案的内容布局
- menuconfig中增加可配置各个section的位置
- OneStep：Windows环境支持mb、mc命令；
- LVGL新增dashboard demo
- 移植Luban中的userid库
- 新增方案：d12x-hmi
- 新增NAND器件：XCSP1AAPK-IT
- 新增第三方包：libmodbus、sqlite、gif、
- 新增示例：test-multipwm、test-qep、test-keyadc
## 优化 ##
- 优化MP4格式的兼容性
- 完善一些特殊包的PPS、Slice处理
- 修正CPU loading的计算
- 优化stop时的资源释放处理
- Kconfig.board：简化依赖关系，每颗SoC只需要维护一份
- Eclipse：支持menuconfig方式进行动态配置SDK
- DMA：优化写内存后的同步处理
- RTP：优化event的上报流程，响应更快；保存校准文件到/data/config目录
- USB：优化UDC的枚举流程，提升兼容性
- SPI：优化多任务并发时的等待机制
- DVP：优化中断的处理流程
- LVGL增加GE透明功能的宏控制
- LVGL旋转支持逆时针、顺时针
- LVGL完善旋转后的搬移处理
- LVGL适配RTP触屏
- 工具链：精简其中的库文件，减小体积
- OneStep：完善Windows环境的路径切换处理
## 修改 ##
- I2C：支持发送restart信号
- CAN：修正工作模式的设置流程
- USB：修正ADB的printf()异常问题；改善ADB连接的稳定性
- GPAI：修正校准参数和计算过程
- NFTL：修正一处内存泄漏
- WiFi：修正rtl8189初始化过程中的异常



# V1.0.3 #
## 新增 ##
- 新增支持FreeRTOS内核
- 新增D12x
- NAND方案：全平台都已支持NFTL
- 支持OTA升级方案
- 新增支持ADB
- 新增支持HID Device，优化Custom IO Demo可接收图片、视频文件
- MPP：D13x、D12x支持MJPEG的解码
- PWM：支持输出指定个数的脉冲信号
- 新增屏支持：MIPI ili9488、LCD st7701s
- 新增WiFi支持：rtl8733、rtl8189
- 新增ESMT等厂家的多款NAND支持
- 新增CTP支持：ft7411、gsl1680，并增加相应的测试示例
- 新增Codec支持：tlv320
- LVGL Demo：支持动态旋转、缩放、任意角度旋转、多国语言、GIF图片
- 新增示例：test_fb、draw_line、test_i2s_loopback
## 优化 ##
- D12x: 仪表盘Demo优化达到58帧/s
- UART烧写：优化速率（最高可达3Mbps）和稳定性
- D21x：功耗优化
- 增强刷Cache时的对齐检查
- 优化调度入口的处理流程
- FATFS：支持sparse格式
- 优化PSRAM的稳定性；将PSRAM初始化统一放在PBP中
- 支持USB3.0的U盘
- 优化Device驱动的Buf性能
## 修改 ##
- CAN：修正HDR参数
- RTP：修正UP事件丢失的问题
- USB：修正U盘压力测试中出错的问题；
- RTP：将校准数据保存到文件中
- 默认关闭PM（功耗管理）功能
- 修改 application/os 目录名称为 application/rt-thread
- cherryUSB升级为v1.0.0版本
- gt911：修正多点触摸时的异常问题
- I2C：修正收发长报文的异常问题
- zlib解压缩时使用轮询方式
- 完善对B帧数据的处理
- 优化单曲循环播放时的切换机制
- Audio：优化播放流程，改善播放数据的完整性；设置最大音量为0db
- PWM：优化默认值、完善shadow寄存器的流程

# V1.0.2 #
## 新增 ##
- 支持动态APP加载
- 电源管理：支持休眠唤醒流程，新增light sleep模式
- 启动：NOR支持XIP模式、支持eMMC启动和烧写
- UI新增支持AWTK
- NAND：支持NTFL、FatFS
- SD卡：支持热插拔
- Audio：支持暂停功能
- Network：新增ping命令、支持MQTT协议
- USB：支持cherryUSB V0.10.0，USB Host和Device功能可用
- SPI：支持Slave模式（仅提供说明给gx客户）、Bit模式
- MPP：支持JPeg video
- Display: 支持SRGB、Gamma调节、PWM背光
- RTP：支持五点校准算法
- TSen：支持温度校准
- GPAI：支持自动校准
- HRTimer：适配D13x
- 支持UART烧写
- 支持OTA升级
- 支持ENV分区
- OneStep新增命令：add-board、rm-board、mc（clean + make）、
- 新增示例：test-tsen、test-gpai、test-rtp、test_gpio、test_i2c、test_ce、test_pm、test_rtc、ge_test
- 新增board：D13x demo88 NOR/NAND、D12x demo68 NOR/NAND



# V1.0.1 #
## 新增 ##
- 添加awtk支持
- 添加OTA功能

# V1.0.0 #
## 新增 ##
- 初始稳定版
- 支持D21X,D13X

