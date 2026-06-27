# MTOP 带宽监控 (test-mtop)

## 一句话描述

MTOP 模块带宽监控工具，实时显示各端口组的读写带宽使用情况（类似 Linux top）。

## 调用关系

```
MSH Shell → test-mtop → AIC MTOP 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_mtop.c` | MTOP 带宽监控主程序 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_MTOP_DRV_TEST` | 启用 MTOP 驱动测试命令 | y | AIC_MTOP_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable mtop driver test command
```

## 使用说明

### `mtop` — MTOP 带宽监控

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-n <num>` | 更新次数（0 为无限） |
| `-d <sec>` | 更新间隔（秒） |
| `-h` | 显示帮助 |

**使用示例：**

```bash
# 默认持续监控带宽
mtop

# 更新 10 次，每次间隔 2 秒
mtop -n 10 -d 2

```

## 依赖说明

- **上游依赖**：AIC MTOP 驱动
- **下游被依赖**：无
