# 加密引擎测试 (test-ce)

## 一句话描述

CE（Crypto Engine）硬件加密引擎测试，同时提供软件 AES-ECB 实现作为性能对比。

## 调用关系

```
MSH Shell → test-ce → AIC CE 驱动
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_ce.c` | CE 硬件加密引擎测试 |
| `test-soft-aes-ecb.c` | 软件 AES-ECB 实现测试 |
| `soft-aes-ecb.c` | 软件 AES-ECB 算法实现 |
| `soft-aes-ecb.h` | 软件 AES-ECB 头文件 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_CE_DRV_TEST` | 启用 CE 驱动测试命令 | n | AIC_CE_DRV |
| `AIC_SOFT_AES_TEST` | 启用软件 AES ECB 测试命令 | n | RT_USING_FINSH |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable CE driver test command / Enable Soft AES ECB test command
```

## 使用说明

### `test_ce` — 加密引擎测试

支持 Hash、对称加密、全量测试和稳定性测试。

**参数说明：**

| 参数 | 说明 |
|------|------|
| `hash` | 执行硬件 Hash 测试 |
| `symm` | 执行硬件对称加密测试 |
| `all` | 执行 Hash + 对称加密测试 |
| `stability` | 循环执行全量测试（按 Ctrl+C 退出） |

**使用示例：**

```bash
# 仅测试 Hash
test_ce hash

# 仅测试对称加密
test_ce symm

# 执行全部测试
test_ce all

# 稳定性压力测试
test_ce stability
```

### `test_aes_ecb` — 软件 AES-ECB 测试

执行软件 AES-128-ECB 加密/解密验证（无参数）。

```bash
test_aes_ecb
```


## 依赖说明

- **上游依赖**：AIC CE 驱动
- **下游被依赖**：无
