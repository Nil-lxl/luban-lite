# 芯片 ID 管理 (test-userid)

## 一句话描述

UserID（芯片唯一标识）管理命令，支持初始化、dump、导入导出、删除等操作。

## 调用关系

```
MSH Shell → test-userid → AIC UserID 模块
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_userid.c` | 芯片 ID 管理命令 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_USERID_BARE_TEST` | 启用 UserID 测试命令 | n | LPKG_USING_USERID |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
    └─ Drivers examples
      └─ Enable userid test command
```
## 使用说明

### `userid` — 芯片 UserID 管理命令

**参数说明：**

| 参数 | 说明 |
|------|------|
| `userid list` | 列出所有 UserID 条目 |
| `userid init` | 初始化 UserID 区域 |
| `userid dump <name> [offset] [size]` | dump 指定 UserID 数据 |
| `userid size <name> [addr]` | 查询/设置 UserID 大小 |
| `userid remove <name>` | 删除指定 UserID |
| `userid import <addr>` | 从内存地址导入 UserID |
| `userid export <name> <addr>` | 导出 UserID 到内存地址 |

**使用示例：**

```bash
# 列出所有 UserID
userid list

# 初始化 UserID 存储区域
userid init

# dump chipid 数据
userid dump chipid

# 删除 chipid 条目
userid remove chipid

```

## 依赖说明

- **上游依赖**：AIC UserID 模块
- **下游被依赖**：无
