# MTD 文件加载测试 (test-mtd-file)

## 一句话描述

从 MTD 设备读取指定大小数据并写入到文件，验证 MTD 到文件的数据传输。

## 调用关系

```
MSH Shell → test-mtd-file → RT-Thread MTD → POSIX FS
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_mtd_load_file.c` | MTD 数据加载到文件 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_MTD_LOAD_FILE_TEST` | 启用 MTD 数据加载到文件测试 | n | `RT_USING_MTD_NOR \|\| RT_USING_MTD_NAND` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable load mtd data to a file
```

## 使用说明

### `test_mtd_load_file` — MTD 数据加载到文件

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<mtd_name>` | MTD 设备名称 |
| `<file_size>` | 读取数据大小（字节，支持 0x 前缀） |
| `<file_name>` | 输出文件路径 |

**使用示例：**

```bash
# 从 MTD os 分区读取 16KB 到 os.bin
test_mtd_load_file os 0x4000 os.bin

# 从 MTD os 分区读取 16KB 到 spl.bin
test_mtd_load_file os 0x4000 spl.bin

```

## 依赖说明

- **上游依赖**：RT-Thread MTD NOR/NAND, POSIX 文件系统
- **下游被依赖**：无
