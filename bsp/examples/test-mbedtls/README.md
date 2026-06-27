# mbedTLS 加密测试 (test-mbedtls)

## 一句话描述

mbedTLS 加密库功能测试，包含 SHA512、AES-CTR/CBC/GCM、随机数生成等算法验证。

## 调用关系

```
MSH Shell → test-mbedtls → mbedTLS 库
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_mbedtls.c` | mbedTLS 测试入口与调度 |
| `crypto.c` | 加密算法测试实现 |
| `crypto.h` | 加密算法头文件 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `LPKG_USING_MBEDTLS` | 启用 mbedTLS 加密库（通过 packages 选择） | n | - |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ RT-Thread packages
    └─ security packages
      └─ mbedTLS (via packages menuconfig)
```
## 使用说明

### `mbedtls` — mbedTLS 加密算法测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<subcommand>` | 子命令：sha512 / aesctr / aescbc / aesgcm / random / all / help |

**使用示例：**

```bash
# 显示帮助信息
mbedtls help

# 测试 SHA-512 哈希
mbedtls sha512

# 测试 AES-CTR 加解密
mbedtls aesctr

# 测试 AES-CBC 加解密
mbedtls aescbc

# 测试 AES-GCM 加解密
mbedtls aesgcm

# 测试随机数生成
mbedtls random

# 运行全部测试
mbedtls all

```

## 依赖说明

- **上游依赖**：mbedTLS 库（packages/third-party/mbedtls/）
- **下游被依赖**：无
