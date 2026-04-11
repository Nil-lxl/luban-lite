# V1.2.3 #
## 新增 ##
- LVGL：
  - 支持APNG格式
  - 增加FreeType的cache管理接口
  - SPI屏增加旋转功能
  - 播放器适配了V8版本
  - 播放器增加dump调试功能
  - 播放器支持播放速度的设置
- MPP：
  - 混音播放时，支持每一路单独设置音量
  - 增加GE的简化版示例
  - 增加parser的测试示例
  - D21x的视频播放也支持ext render方式，支持循环播放
  - 增加更多的调试命令
- USB副屏：
  - 启用新的PID管理机制
  - MacOS驱动更新为V1.0.0
  - Linux驱动更新为V1.1.2
  - 支持获取Device的镜像版本号
  - 支持将触屏模拟为鼠标
- USB：
  - USBD：支持slave模式使用专用FIFO
  - USBD：UVC：增加控制接口的描述配置
  - USBH：增加TD buffer的管理机制
  - USBH：支持对不同串口外设的管理
- Display：
  - 支持双SPI屏幕的显示
  - 读取DSI屏幕支持设置最大的数据size
- CherryUSB：升级为V1.5，原V1.0.0继续保留（可手动切换）
- 编译：增加V3.2.0版本的工具链，可支持更完善的C++特性
- FS：再增加一个User FS分区的配置项
- SDIO：增加SDIO manage模块，增强初始化流程的并发管理
- G73：增加XIP模式的方案配置
- XIP：支持运行时的写Flash操作
- DFS：支持块设备挂载的优先级管理
- littlefs：支持挂载多个设备
- 扫码Demo：支持从UART输出扫码结果
- OTA：增加读数据的校验接口
- RTT LwIP：支持MQTT功能
- SPIENC：支持检查Key的CRC32值
- OneStep：‘buildall’命令中会显示方案序号，方便查看整体进度
- test_audio：增加I2S同时播放和录音的示例
- test_dvp：增加wait模式（不再新创建测试线程）
- i2c-tools：支持软件I2C模式
- 烧写：增加对2个plane的Flash支持
- 编译：内置cmake工具，增强Linux和Windows环境的一致性
- 新增软件组件：aic-profiler
- 新增器件：
- WiFi：txw901（64bit）
- NAND：ZB35Q01/02/04CYIG、XCSP1xXPK-IT
- Panel：gc9108
- Touch：ili2117
- Audio Codec：es8311、jy6311
- Camera：xs9950

## 优化
- LVGL：
  - 优化播放器的帧同步处理
  - 优化SPI发送timeout的处理
  - lv-demo：优化导航页面的流畅度
  - 增强对缩放比例的有效性检查
- MPP：
  - 录像功能：完善功能、并优化性能
  - 优化LVGL播放时的尾帧处理
  - 优化延时控制测试以避免丢帧
  - 优化VE对eptb的兼容处理
  - 优化MOV解析的异常处理
  - 完善APNG格式解码的兼容处理
  - 优化ge_test的内存管理的异常分析方法
  - 完善jpeg_encode_test中的异常处理流程
- NAND：
  - 增强数据出错时的容错处理
  - 增强对只读分区的数据安全处理
- USB副屏：
  - 优化复合设备的并发保护
  - 优化U盘模式的切换逻辑
- USB：
  - 优化MSC的初始化流程
  - USBD：优化UVC的数据传输性能
  - 优化一些特殊外设的兼容性处理
  - USBH：增强描述符的保护检查
- XIP：优化rodata分区的数据处理
- FatFS：优化目录扫描的性能
- UART：增强FIFO配置参数的有效性检查
- QSPI：
  - 优化初始化、发送的中断处理流程
  - 增强DMA传输时的数据对齐检查
- NFTL：优化OOB、ECC的兼容处理
- SDMC：
  - 优化频繁插拔卡的处理流程
  - 优化相位参数的配置流程
- DVP：优化DVP的关闭流程
- Audio：优化某些情况的pop音处理
- OTA：优化配置文件的解析处理
- RTC：
  - 起始时间从1970-1-1改为2020-5-20，增加50年的范围
  - 优化低电压模式下的供电配置
- D12x：优化eMMC方案中的PSRAM配置参数
- PBP：优化不同型号PSRAM参数的管理方法
- Watchdog：优化中断回调的调用方式
- GPIO：优化GPIO中断的处理流程
- AiPQ：简化测试功能
- LwIP：优化DHCPD时的DNS释放处理
- WiFi：
  - 优化对SAL接口的兼容处理
  - hugeic：优化内存的释放处理；支持SDIO的重复初始化
  - aic8800：支持WLAN dev可配置；支持重复初始化
- 打包：
- 完善对分区大小的有效性检查
- 完善对loader的异常检查
- 打包FatFS的使用情况信息
- 启动：完善对Ctrl+C按键的检测流程
- cpu_load：支持线程的动态销毁情况

## 修改
- LVGL：
  - AiUIBuilder更名为UIBuilder
  - V9：修正BMP某些情况下的小概率出错
  - V9：修正GIF解码的资源释放流程
  - demo-hub：默认打开播放器相关功能
  - 修正APNG播放器的DMA操作流程
  - 修正电梯Demo的显示异常
  - 修正播放器某些情况下的视频size处理错误
  - 修正播放器的一处内存泄漏
  - 修正播放器UI线程中的一处死锁风险
  - 修正播放器的播放区域异常
  - 修正播放器stop操作的概率性异常
  - 修正播放器在某些情况下的视频卡死问题
  - 修正lv_table的异常处理
  - 修正资源释放的处理流程
  - 解决snapshot功能某些情况下的卡死问题
  - 删除 vscode_simulator demo
- MPP：
  - 修正高度为1时的VE处理流程
  - 修正MJPEG解码中的crop处理
  - 修正JPEG解码在旋转场景中的裁剪处理
- NAND：修正GD5FxGM 系列的ECC参数
- NOR： 使用FatFS时禁止写功能
- UART：修正Tx 发送时的参数配置
- WRI：修正reboot命令执行中对BOOT_INFO的处理

# V1.2.2 #
## 新增 ##
- USB副屏：
  - 支持在Host端完成Scale
  - Monitor名称可定制
  - 支持物理按键操作
  - Windows Driver V1.1.0：优化多处异常处理
  - MacOS Driver V0.9.7: 优化容错处理，支持和Host电源状态的同步
- LVGL：
  - 新增SPI扩展屏的显示Demo，支持双屏异显
  - 新增两种歌词特效
  - 播放器控件：支持在背景播放视频；适配到模拟器
  - 播放器Demo：支持APNG格式解码
  - 增加APNG Demo
  - 支持热插拔的鼠标设备
- MPP：
  - 支持两路音频的混音播放
  - 增加APNG解码支持
- USB：
  - Host支持CDC NCM、USB serial、HID
  - Device支持CDC ECM
- Display：
  - 增加开机时黑屏的选项
  - UI层增加blending的使能接口
- DVP：支持对输入的视频进行裁剪
- aic8800：新增P2P、monitor功能
- realtek8733bs：新增P2P功能
- AiPQ：连接时打印当前的显示接口类型
- LwIP：兼容IPv6
- OTA：支持单独升级data或rodata分区
- aicupg：增加SPIENC的调试模式
- SPI：D13x SPI2支持Quad模式
- Touch：支持动态的区域裁剪
- RTP：支持自动获取X、Y方向的板级电阻值
- CAN: 添加UDS协议栈，以及基于UDS协议栈的OTA功能
- 新增软件组件zipfs，支持从zip文件挂载文件系统
- 新增PBus驱动，以及示例test-pbus
- 新增封装型号：G730BEU
- 新增器件：
  - Panel：ST77916、ST77912、LT8911EXB
  - WiFi：txw901
  - Camera：N5
  - CTP：jd9366
  - NOR：EFM25F128A
  - NAND：ZB35Q02B、ZB35Q04B
- 示例：增加SPI、NAND、NOR访问相关的多个示例、mbedtls示例
- OneStep：增加cs|csys命令，可快捷进入sys目录
- list_mutex命令：增加等待线程的名称显示
- 第三方包：BT协议栈nimble、UDS协议、CPU_load、miniz
- 示例：
  - test-filesystem：增加读文件的压力测试
  - test-mmc：增加读写速度的展示

## 优化 ##
- USB副屏：
  - 优化旋转时的触屏事件处理
  - 优化播放视频时的模式切换处理
  - 连接PC不同USB端口会识别为不同的设备
  - 优化USB热插拔的处理
- USB：
  - Host：优化UVC在高码率时的处理，以及异常处理
  - Host：优化MSC的挂载处理
  - Host：增加U盘的兼容性
  - Device：优化复合设备的处理逻辑
- LVGL：
  - 优化FreeType在多任务并发时的处理
  - 优化播放器的分辨率动态调整处理
  - 优化歌词特效快速切换的处理
  - 优化tick callback的使用方法
  - V8：优化大图的显示处理
  - 优化播放器在个别场景下的帧同步处理
- MPP：优化播放器的性能、稳定性
- DVP：优化在Buf资源不够用的异常处理
- Display：完善MIPI DSI异常情况的log
- rtt：优化tick计数的溢出处理
- SPI：
- NOR：优化写保护的处理；优化异常掉电时的保护处理
- NAND：优化坏块的标记处理
- Slave：支持Master的非对齐访问
- 优化通信异常的保护机制
- realtek：优化多个异常情况的处理；完善auth相关的接口
- OTA：完善SPINAND的坏块处理
- aicupg：兼容block是128的情况
- BootLoader：优化FATFS的兼容性；优化U盘升级的异常处理
- 编译：完善BootLoader的检查和报错
- Scons：支持固定的链接次序

## 修改 ##
- D13x：打开Cache的预取功能，提升性能
- LVGL：修正播放器在旋转显示时的处理
- UART：完善参数配置的流程，先获取再设置
- D21x QFN88/100：修正RGB888模式的pinmux配置
- G73：barcode：修正内存的申请参数
- SPI：
  - 解决打开SFUD_USING_QSPI后的编译问题
  - 修正数据块超过8K的写处理
- LwIP：
  - 调整框架中的默认参数，更方便使用
  - 完善模式动态切换时的处理
  - 分离DNS服务的处理逻辑
- WiFi：完善关闭WiFi后的SDIO状态处理
- mbedtls：关闭RSA CRT算法
- env：删除不再使用的Python27_32目录
- OneStep：Windows环境：修正buildall命令的warning统计处理



# V1.2.1 #
## 新增 ##
- USB 副屏：
  - Windows驱动更新为V1.0.6：支持高画质模式
  - macOS驱动更新为V0.9.2：支持自动launcher
  - 增加待机模式的处理
  - 支持无LVGL时的全屏旋转
  - 是否上报序列号可配置
  - HID：支持在第二块屏幕上设置是否使能HID
- 歌词特效：
  - 新增两个动画特效
  - 支持字体随机
  - 支持泰国字体的处理
- MPP：
  - 增加保存FrameBuffer数据的demo
  - 支持AAC文件的解析
  - Player：支持独立播放一个wav文件
  - audio player：支持两路音频动态切换
  - D13x也支持录像功能
- LVGL：
  - 新增Demo：串口接收、input测试、RTP校准（支持重新校准）
  - 新增控件：Video window控件、swipe控件
  - 支持编码器作为输入设备
  - Player：支持缩放显示；支持视频切换
- USB：Device：HID touch/MSC 增加注销流程
- FatFS：增强掉电场景的保护处理
- NFTL：支持烧录器方式
- hrtimer：支持多通道
- WiFi：realtek支持基于配置文件完成自动配置
- 启动：支持预先配置一些GPIO的状态
- 打包镜像：支持解压、再重新打包
- test-mmc：增加自动扫描采样相位的功能
- 新增器件：
  - Panel：CHIPONE CO5300、NV3051、FL7707、FT8201
  - NAND：ZB35Q01B
  - Touch：chsc5xxx、chsc6xxx
  - Camera：OV9281、TP9951
  - 编码器：HEC40
- 新增示例：test-tcp、test-udp、ge fillrect/bitblit/rotation/alpha bend/format convert
- 新增方案配置：d12x-demo68-mmc

## 优化 ##
- 歌词特效：
  - 解决特效切换时的滚动慢问题
  - 优化居中显示的处理
  - 增加shake特效的设置
- MPP：优化H264码流的兼容性
- USB Host：优化MSC的disconnect处理
- USB OTG：优化Host切换状态时的处理流程
- NAND：
  - 优化block层的多线程并发处理
  - 优化读性能
  - 优化ECC、OOB的接口实现
- SPI：支持异步模式的访问；兼容RT-Thread SPI框架的模式参数
- mbedtls：优化与不同版本LwIP配合的兼容性
- 烧写：优化UART的烧写速度；优化NOR的擦写速度
- PM：优化休眠唤醒的接口和流程，支持power key控制模式
- aic8800：增加BT接口，并提升稳定性
- 打包镜像：更新truncate工具，支持更大容量分区

## 修改 ##
- USB副屏：
  - OSD：调整“显示”的子菜单布局，包含“旋转”等
  - 完善第二块屏的初始化流程
  - 重构复合设备的管理流程
- LVGL：
  - V9：D13x 方案中只使用SRAM空间
  - 简化用户内存的配置方法
  - demo-hub中默认播放器改为ffmpeg
  - demo-hub去掉FreeType的支持
  - player：完善pause的处理流程；修正某些情况下的丢帧问题；完善切换视频源的异常处理
  - 86box：修正timer的溢出判断
  - 优化文件系统的mount同步处理
- MPP：
  - audio player：修正某些情况下的stop状态同步问题
  - player：默认采用VE直接填充FrameBuffer的方式
  - ve：调整Jpeg旋转的参数配置
  - AAC：完善buf管理的处理
- OTA：修正检测到有坏块后的处理流程
- QSPI0：默认IO采用上拉配置
- SFUD：修正100MHz标准SPI模式的Fast read命令
- DFS：完善异常情况下的资源释放处理
- 编译：修正某些情况下的Kconfig重复引用
- 动态内存：管理策略统一改为best模式
- D12x：统一采用默认的Efuse配置参数
- D21x：增加Idle线程栈的大小
- SDMC：统一SDMC的设备名称规则
- UART：完善奇偶校验的异常处理
- CAN：完善某些情况下的异常处理
- 烧写：修正RAM资源小的情况下的分区CRC计算
- OTA：完善SDNAND的支持；设置env分区为只读属性
- WiFi：完善WLAN框架中的异常处理
- AiPQ：完善Panel多配置情况下的处理
- Eclipse：改善脚本在Python2环境中的兼容性
- FreeRTOS：修正free命令的处理
- TP2825：恢复TEST_MODE的处理代码
- OneStep：修正list_module命令的处理



# V1.2.0 #
## 新增 ##
- USB：
  - 增加OHCI支持
  - Host：支持多interface的HID设备
  - 增加Host UVC支持
  - Host UVC支持MJpeg格式
- LVGL：
  - 增加Camera控件
  - 增加图片滚动控件
  - 增加aic_player控件，以及aic_player_demo，兼容单、双Buffer显示，支持V8、V9
  - 支持BMP图片解码
- MPP：
  - 增加音频的循环播放功能
  - 增加VE/GE test的配置选项
- 正式支持VSCode Luban-Lite插件
- defconfig：支持增量保存，需要维护的配置项省掉95%
- AiPQ：支持通过UART调试DSI屏幕
- SPI NAND：支持multi-plane的设备
- PM：支持触屏唤醒
- QEP：增加count清零的接口
- CIR：新增NECX协议支持
- baremetal：支持DFS的mkfs功能
- aicupg：增加安全固件的传输和烧写
- tools：增加一个Panic log分析脚本
- FreeType：增加OTF格式支持
- barcode：增加LED灯控制
- 新增组件：llm（大语言模型）
- 新增SoC型号：d123x
- 新增器件：
  - Touch：axs15260
  - WiFi：aic8800dw
  - Camera：SC035HGS、SC031IOT、OV9281、TP2850
##优化##
- USB副屏：
  - 更新Windows驱动V1.0.2、Linux驱动V1.0.2、MacOS驱动V0.4.0，性能有优化
  - OSD：完善Video播放的资源释放处理
  - 支持动态旋转
  - 休眠时显示logo图片
  - 优化复合设备的配置参数和切换流程
  - 优化非Cache的API访问
  - 优化AMD显卡的兼容性
- MPP：
  - 优化MP3、WAV、MP4文件解析的兼容性
  - 优化动态调音量的处理
  - 优化播放器的内存占用
  - TS格式支持AAC
  - 优化音频脏数据、空数据的处理
  - 优化音视频同步的策略
- DVP：优化隔行模式的处理流程
- BootLoader：支持关闭aicupg功能
- OTA：检查坏块前先执行擦写
- ABSystem：优化更新系统的逻辑
- littlefs：支持长文件名；当文件系统满时抛出异常
- I2C：收发数据完成后关闭I2C；针对不同速率优化超时条件
- i2c-tools：支持16bit的寄存器读写
- ADC：优化校准参数和计算方法
- NOR：自适应的调整Rx delay模式
- UART：
  - 收发数据都支持中断方式
  - 优化设置运行时设置波特率的处理
- RTC：增加低功耗模式的配置选项
- GPIO：优化GPIO中断的状态处理
- ENV：当ENV数据没有变化的时候，不再写Flash
- 编译：支持整个bsp目录生成一个静态库
- 启动logo：完善SDK的配置选项
- barcode：优化数据的Cache处理
- benchmark：适配baremetal模式
- VSCode配置：排除掉output、toolchain目录
##修改##
- LVGL：
  - 优化fake图片资源的处理
  - 修正lv_arc的区域裁剪处理
  - 修正FreeRTOS环境的realloc处理
  - 修正旋转时的BUF size处理
  - 完善TP回调中的异常处理
- USB：
  - MSC：修正baremetal的异常处理，支持“弹出”操作
  - 当键盘Device时增加超时处理
- MPP：
  - 修正多线程时的内存资源保护
  - 修正Seek操作的位置计算
- DVP：在D13x中提升工作频率到200MHz
- CE：修正MD5算法的尾数处理
- DDR3：修正工作频率为600MHz
- SPI NAND：
  - 修正RODATA的坏块管理
  - 增强BBT扫描的边界保护
- SPI NOR：修正写数据长度超过8KB的处理
- GMAC：使用ChipID的MD5值生成默认的MAC地址
- EPWM：修正脉冲输出的控制模式参数
- QEP：修正中断函数的状态处理
- eFuse：默认关闭eFuse的写功能；增强eFuse写保护的逻辑
- PM：
  - LVGL任务在运行时将阻止进入休眠
  - test_pm：修正自动唤醒的处理
  - 修正GPIO中断唤醒的处理流程
- 编译：修正Win7系统中生成镜像时的错误
- 打包镜像：修正FatFS镜像的分区大小匹配判断
- RTP：兼容没有配置Y plate参数时的压力计算
- UART：
  - 修正G73x的自动波特率参数选择
  - 修正DMA模式的Rx FIFO设置
- 烧写：
  - 当FB申请失败的时候避免进入异常
  - 修正安全烧写时的CRC校验错误问题
  - 修正tweak解析错误问题
- DM：修正编译错误
- pinmux冲突检查：兼容PM pin的检查
- d21x_demo128_nand：增加NFTL配置



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

