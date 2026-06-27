# 捕获定时器测试 (test-cap)

## 一句话描述

输入捕获（Capture）定时器功能测试，验证定时器捕获模式。

## 调用关系

```
MSH Shell → test-cap → AIC CAP 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_cap.c` | 捕获定时器测试主程序 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_CAP_DRV_TEST` | 启用 CAP 驱动测试命令 | n | AIC_CAP_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable CAP driver test command
```

## 使用说明

### `test_cap` — 输入捕获测试

对指定通道进行输入捕获，测量信号频率和占空比。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `channel` | 捕获通道号 |
| `[second]` | 测试时长（秒，默认 10） |

**使用示例：**

```bash
# 测试通道 0，默认 10 秒
test_cap 0

# 测试通道 1，持续 5 秒
test_cap 1 5
```


## 依赖说明

- **上游依赖**：AIC CAP 驱动
- **下游被依赖**：无
