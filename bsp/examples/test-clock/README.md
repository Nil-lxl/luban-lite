# 时钟管理测试 (test-clock)

## 一句话描述

CMU（Clock Management Unit）时钟管理模块测试，验证时钟频率配置、切换与查询。

## 调用关系

```
MSH Shell → test-clock → AIC CMU 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_clock.c` | CMU 时钟管理测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_CMU_DRV_TEST` | 启用 CMU 驱动测试命令 | n | AIC_CMU_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable CMU driver test command
```

## 使用说明

### `test_clock` — 时钟管理单元测试

支持查询所有时钟信息、按 ID/名称查询、设置频率、使能/禁用时钟。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-a` | 打印所有时钟信息 |
| `-f <id\|name>` | 按 ID 或名称查询时钟信息 |
| `-p <id\|name>` | 查询时钟父节点 |
| `-s <id\|name> <rate>` | 设置时钟频率 |
| `-e <id\|name>` | 使能时钟 |
| `-d <id\|name>` | 禁用时钟 |
| `-h` | 显示帮助信息 |

**使用示例：**

```bash
# 查看所有时钟信息
test_clock -a

# 查询时钟 ID 0 的信息
test_clock -f 0

# 按名称查询
test_clock -f sdmc0

# 设置 sdmc0 频率为 100MHz
test_clock -s sdmc0 100000000

# 使能时钟
test_clock -e sdmc0

# 禁用时钟
test_clock -d sdmc0
```


## 依赖说明

- **上游依赖**：AIC CMU 驱动
- **下游被依赖**：无
