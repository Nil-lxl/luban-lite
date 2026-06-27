# 文件 CRC32 校验 (test-crc32)

## 一句话描述

文件 CRC32 校验值计算工具，对指定文件计算并输出 CRC32 结果。

## 调用关系

```
MSH Shell → test-crc32 → POSIX 文件系统
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `file_crc32.c` | 文件 CRC32 计算程序 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_FILE_CRC32_TEST` | 启用文件 CRC32 测试命令 | n | RT_USING_POSIX_FS |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable file CRC32 test command
```

## 使用说明

### `file_crc32` — 计算文件 CRC32

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<filename>` | 待计算的文件路径 |

**使用示例：**

```bash
# 计算 test.bin 的 CRC32 值
file_crc32 /sdcard/test.bin

```

## 依赖说明

- **上游依赖**：POSIX 文件系统, RT-Thread Legacy 组件
- **下游被依赖**：无
