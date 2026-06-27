# Monkey 随机事件测试 (test-monkey)

## 一句话描述

Monkey 随机触摸/按键事件注入测试，支持绘图、自由点、录制/回放和组合模式。

## 调用关系

```
MSH Shell → test-monkey → Touch 驱动 + Framebuffer
```

## 目录结构

| 文件 | 职责 |
|------|------|
| `test_monkey.c` | Monkey 随机事件注入测试 |
| `SConscript` | 编译脚本 |

## 配置方法

| Kconfig | 说明 | 默认值 | 依赖 |
|---------|------|--------|------|
| `AIC_MONKEY_TEST` | 启用 Monkey 测试命令 | n | AIC_USING_TOUCH |

通过 `scons --menuconfig` 开启：

```
menuconfig
  └─ Drivers options
      └─ Drivers examples
          └─ Enable Monkey test command
```

## 使用说明

### `test_monkey` — Monkey 随机事件测试

**参数说明：**

| 参数 | 说明 |
|------|------|
| `-D, --mode_draw` | 绘图模式 |
| `-F, --mode_free_point <n>` | 自由点模式，指定点数 |
| `-R, --mode_record [path]` | 录制模式 |
| `-P, --mode_play [path]` | 回放模式 |
| `-W, --mode_combination` | 组合模式 |
| `-T, --time_interval_line <ms>` | 线条时间间隔（ms） |
| `-U, --time_interval_point <ms>` | 点时间间隔（ms） |
| `-N, --total_lines_drawn <n>` | 绘制总线条数 |
| `-A, --total_points_drawn <n>` | 绘制总点数 |
| `-x, --comb_line_x <n>` | 组合模式起始 X |
| `-y, --comb_line_y <n>` | 组合模式起始 Y |
| `-r, --comb_line_swipe_right <n>` | 右滑长度 |
| `-e, --comb_line_swipe_left <n>` | 左滑长度 |
| `-u, --comb_line_swipe_up <n>` | 上滑长度 |
| `-d, --comb_line_swipe_down <n>` | 下滑长度 |
| `-h, --help` | 显示帮助 |

**使用示例：**

```bash
# 绘图模式：50 条线、200 个点
test_monkey -D -N 50 -A 200

# 自由点模式：100 个随机点
test_monkey -F 100

# 录制触摸事件到文件
test_monkey -R /tmp/events.dat

# 回放触摸事件
test_monkey -P /tmp/events.dat

```

## 依赖说明

- **上游依赖**：Touch 驱动, Framebuffer 设备
- **下游被依赖**：无
