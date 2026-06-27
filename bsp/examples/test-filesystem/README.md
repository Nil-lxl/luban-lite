# 文件系统测试 (test-filesystem)

## 一句话描述

文件系统综合测试程序集，包含 C 标准文件操作、POSIX 文件操作示例和多线程压力测试。

## 调用关系

```
MSH Shell → test-filesystem → RT-Thread DFS / POSIX FS
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_filesystem.c` | 文件系统基础测试 |
| `c_fs_example.c` | C 标准库文件操作示例（fopen/fwrite/fread） |
| `posix_fs_example.c` | POSIX 文件操作示例（open/write/read） |
| `test_fs_thread.c` | 文件系统多线程并发读写压力测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_FILE_SYSTEM_TEST` | 启用文件系统测试命令 | n | RT_USING_FINSH |
| `RT_USING_POSIX_FS` | 启用 POSIX 文件系统 | y | - |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
    └─ Drivers examples
      └─ Enable filesystem test command
```
## 使用说明

### `fs` — 文件系统基础测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `help` | 显示帮助信息 |
| `write <file> [size]` | 写入测试（可指定文件大小） |
| `read <file>` | 读取测试（read 函数） |
| `fread <file>` | 读取测试（fread 函数） |
| `open <file>` | 打开速度测试 |
| `close` | 关闭速度测试 |

**使用示例：**

```bash
# 自动写入多种大小文件
fs write data auto

# 自动读取
fs read data auto

# 指定文件和大小写入
fs write data/testfile.txt 2048

# 读取指定文件
fs read data/testfile.txt
```

### `c_fs_demo` — C 标准文件操作示例

```bash
c_fs_demo
```

### `posix_fs_demo` — POSIX 文件操作示例

```bash
posix_fs_demo
```

### `fs_read_thread` — 文件系统压力测试

```bash
fs_read_thread
```


## 依赖说明

- **上游依赖**：RT-Thread DFS, POSIX 文件系统
- **下游被依赖**：无
