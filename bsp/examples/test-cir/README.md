# 红外遥控测试 (test-cir)

## 一句话描述

CIR（Consumer Infrared）红外遥控收发测试。

## 调用关系

```
MSH Shell → test-cir → AIC CIR 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_cir.c` | 红外遥控收发测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_CIR_DRV_TEST` | 启用 CIR 驱动测试命令 | n | AIC_CIR_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable CIR driver test command
```

## 使用说明

### `test_cir` — 红外收发测试

支持 NEC、RC5 等协议的红外编码发送与接收解码。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-p <proto>` | 红外协议：`NEC` / `RC5` 等 |
| `DATA` | 发送的红外编码（十六进制） |
| `-h` | 显示帮助信息 |

**使用示例：**

```bash
# NEC 协议发送 0x00ff23
test_cir -p NEC 0x00ff23

# RC5 协议发送 0x81b
test_cir -p RC5 0x81b
```


## 依赖说明

- **上游依赖**：AIC CIR 驱动
- **下游被依赖**：无
