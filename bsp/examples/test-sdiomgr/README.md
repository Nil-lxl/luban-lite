# SDIO 管理器测试 (test-sdiomgr)

## 一句话描述

SDIO Manager（SDIO 管理器）测试程序，验证 SDIO 设备的管理功能。

## 调用关系

```
MSH Shell → test-sdiomgr → SDIO 框架
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_sdiomgr.c` | SDIO 管理器测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_SDIO_MGR_TEST` | 启用 SDIO 管理器测试命令 | n | RT_USING_SDIO && !DRIVER_BARE_DRV_EN |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable SDIO mgr test command
```

## 使用说明

本模块不导出 MSH Shell 命令。编译进固件后，会在初始化时自动创建测试线程，等待 SDIO 设备就绪后执行 WiFi Station 模式初始化。

## 依赖说明

- **上游依赖**：RT-Thread SDIO 框架
- **下游被依赖**：WiFi 驱动（SDIO 接口）

> **注意**：本测试模块未导出 MSH 命令，会在初始化时创建一个线程运行管理服务。
