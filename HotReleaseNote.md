# 2025-12-18 #
## 新增 ##
- Demo Board:
  - HOT68-DEMO-A01_V0 MIPI测试板
- Display Driver:
  - MIPI I/F:
    - H016A01
    - H028B23
    - H030B07
    - H034A01
    - H035A27
    - H035B16
    - H035B22
    - H040B24
    - H055A03
    - H080A11
    - H080D16 (MIPI convert dual LVDS)
  - RGB I/F:
    - H035A17
    - H043A7
    - H068B06
- Touch Driver:
  - ST7123 Touch Screen Controller

## 修改 ##
- gif_demo修改为player_demo
- test_demo屏幕测试程序加入灰阶画面
- 无线网RTL8733配置
- 修改ili2511驱动代码,gt911上电时序
- display背光使能gpio打开/关闭

# 2025-1-14 #
## 新增 ##
- Demo Board:
  - HT68-DEMO-A02_V0 演示板
- Display Driver:
  - 添加自定义RGB Display默认屏参:
    - H050A21
    - H068b06
    - H070A20
  - 添加spi控制器代替de驱动spi显示屏初始化代码
    - H011A1 
    - H027A28
- 使用UiBuilder开发的LVGL demo
## 修改 ##