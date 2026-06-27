# zlib 压缩测试 (test-zlib)

测试 zlib 压缩和解压功能。

## 调用关系

```
Bare Shell → test-zlib → AIC ZLIB 驱动 (`bsp/artinchip/drv_bare/`)
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_zlib.c` | 测试实现 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_ZLIB_TEST` | 启用 zlib 测试命令 | n | `LPKG_MPP` |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ 启用 zlib 测试命令
```

## 使用说明

### `zlib_test` — zlib 压缩/解压测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<input_file>` | 输入的压缩文件路径 |
| `<output_file>` | 解压后的输出文件路径 |
| `<buf_len>` | 输出缓冲区大小（必须 ≥ 解压后文件大小） |

**使用示例：**

```bash
# 解压 readme.zlib 到 readme.txt，缓冲区 204800 字节
zlib_test readme.zlib readme.txt 204800
```

## 依赖说明

- **上游依赖**：AIC ZLIB 驱动
- **下游被依赖**：无
