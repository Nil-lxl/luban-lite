# DMA 传输测试 (test-dma)

## 一句话描述

DMA 控制器测试，支持内存拷贝、内存填充等操作的 DMA 加速性能测试。

## 调用关系

```
MSH Shell → test-dma → AIC DMA 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_dma.c` | DMA memcpy/memset 测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_DMA_DRV_TEST` | 启用 DMA 驱动测试命令 | n | AIC_DMA_DRV |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable DMA driver test command
```

## 使用说明

### `test_dma_memcpy` — DMA 内存拷贝测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<length>` | 拷贝数据长度（字节） |

**使用示例：**

```bash
# DMA 拷贝 1KB 数据
test_dma_memcpy 1024

# DMA 拷贝 64KB 数据
test_dma_memcpy 65536

```

### `test_dma_memset` — DMA 内存填充测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<value>` | 填充值 |
| `<length>` | 填充长度（字节） |

**使用示例：**

```bash
# DMA 填充 4KB 为 0xAA
test_dma_memset 0xAA 4096

```

### `test_dma_memcpys` — DMA 内存拷贝流式测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `<length>` | 拷贝数据长度（字节） |

**使用示例：**

```bash
# DMA 流式拷贝 2KB
test_dma_memcpys 2048

```

## 依赖说明

- **上游依赖**：AIC DMA 驱动
- **下游被依赖**：无
