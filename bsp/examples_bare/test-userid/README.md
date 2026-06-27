# 用户 ID 测试 (test-userid)

测试芯片 UserID 的读写和管理功能。

## 调用关系

```
Bare Shell → test-userid → AIC USERID 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_userid.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_USERID_BARE_TEST` | 启用 UserID 裸机测试命令 | n | `AIC_USERID_DRV` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 UserID 裸机测试命令
```

## 使用说明

### `userid` — UserID 读写操作

**子命令说明：**

| 子命令 | 说明 |
|--------|------|
| `help` | 显示帮助信息 |
| `init` | 初始化 UserID 子系统 |
| `list` | 列出所有 UserID 条目名称 |
| `import <addr>` | 从 RAM 地址导入 UserID 数据 |
| `export <addr>` | 导出 UserID 数据到 RAM 地址 |
| `dump <name> [offset] [size]` | Dump 指定条目数据（省略 offset/size 则 dump 全部） |
| `size <name> [addr]` | 获取条目数据大小（可选写入到 RAM 地址） |
| `read <name> <offset> <size> <addr>` | 读取条目数据到 RAM 地址 |
| `write <name> <offset> <size> <addr>` | 从 RAM 地址写入数据到条目 |
| `remove <name>` | 删除指定条目 |
| `writehex <name> <offset> <data>` | 以十六进制字符串写入数据 |
| `writestr <name> <offset> <data>` | 以字符串写入数据 |
| `lock` | 锁定 UserID 分区 |
| `unlock` | 解锁 UserID 分区 |
| `save` | 保存 UserID 数据到存储 |

**使用示例：**

```bash
# 初始化 UserID
userid init

# 列出所有条目
userid list

# 写入十六进制数据
userid writehex testid 0 112233445566778899AABBCCDDEEFF

# Dump 条目数据
userid dump testid

# 保存到存储
userid save

# 锁定分区（不可逆，请谨慎操作）
userid lock
```

## 依赖说明

- **上游依赖**：AIC USERID 驱动
- **下游被依赖**：无
