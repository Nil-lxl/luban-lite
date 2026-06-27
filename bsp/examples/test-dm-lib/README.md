# DM 示例 (test-dm-lib)

## 一句话描述

DM（Dynamic Model）动态模块加载的示例程序。

## 调用关系

```
MSH Shell → test-dm-lib → DM 库
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_dm_lib.c` | DM 库示例主程序 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_DM_LIB_TEST` | 启用 DM 库测试命令 | n | RT_USING_MODULE |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
    └─ Drivers examples
      └─ Enable DM Lib test command
```
## 使用说明

### `test_dm_lib` — DM 库动态加载测试

动态加载 DM 库（.so）并执行库中的测试函数。

**使用示例：**

```bash
# 动态加载 DM 库并执行
test_dm_lib
```


## 依赖说明

- **上游依赖**：AIC DM 库
- **下游被依赖**：无
