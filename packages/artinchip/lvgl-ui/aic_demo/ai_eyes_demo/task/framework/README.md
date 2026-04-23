# 嵌入式应用框架设计文档

## 概述

本框架采用**消息总线（Message Bus）**和**模块化（Module）**设计模式，实现了松耦合的嵌入式应用架构。

### 设计目标

1. **解耦模块**：模块间通过消息总线通信，避免直接调用
2. **易于扩展**：新增模块只需实现模块接口，无需修改其他模块
3. **统一规范**：所有模块遵循相同的接口规范，代码风格统一
4. **易于理解**：清晰的架构设计，便于团队协作

## 架构设计

```
┌─────────────────────────────────────────────────────────────┐
│                      应用层 (Application)                    │
│                  spi_extend_demo.c                           │
└──────────────────────────┬──────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────┐
│                   模块层 (Modules)                           │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ WiFi     │  │ XiaoZhi  │  │   UI     │  │  Audio   │   │
│  │ Module   │  │ Module   │  │ Module   │  │ Module   │   │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘   │
│       │             │             │             │           │
└───────┼─────────────┼─────────────┼─────────────┼───────────┘
        │             │             │             │
        └─────────────┼─────────────┼─────────────┘
                      │             │
        ┌─────────────▼─────────────▼─────────────┐
        │        消息总线 (Message Bus)            │
        │     msg_bus_publish() / receive()        │
        └──────────────────────────────────────────┘
                      │
        ┌─────────────▼─────────────┐
        │      核心框架 (Core)       │
        │  - msg_def.h 消息定义      │
        │  - module.h  模块接口      │
        │  - msg_bus.h 消息总线      │
        └────────────────────────────┘
```

## 核心组件

### 1. 消息定义 (msg_def.h)

定义了系统中所有消息类型和数据结构：

- **消息类型枚举**：MSG_TYPE_WIFI_*, MSG_TYPE_XIAOZHI_*, 等
- **状态枚举**：wifi_state_t, emotion_status_t
- **消息结构体**：app_message_t（统一的消息格式）

### 2. 模块接口 (module.h)

定义了模块的标准接口：

```c
typedef struct module_ops {
    int (*init)(void);
    int (*deinit)(void);
    int (*msg_handler)(const app_message_t *msg);
    const char *name;
    module_id_t id;
} module_ops_t;
```

### 3. 消息总线 (msg_bus.h/c)

负责消息的路由和分发：

- **订阅/取消订阅**：模块可以订阅感兴趣的消息类型
- **发布消息**：模块可以发布消息到总线
- **接收消息**：模块从自己的消息队列接收消息

## 使用流程

### 1. 定义模块

```c
#include "framework/core/module.h"
#include "framework/core/msg_def.h"

/* 模块私有数据 */
typedef struct {
    /* ... */
} wifi_module_priv_t;

/* 消息处理函数 */
static int wifi_msg_handler(const app_message_t *msg)
{
    switch (msg->header.type) {
        case MSG_TYPE_SYS_INIT:
            /* 处理初始化消息 */
            break;
        /* ... */
    }
    return 0;
}

/* 模块初始化 */
static int wifi_init(void)
{
    /* 初始化WiFi */
    return 0;
}

/* 模块去初始化 */
static int wifi_deinit(void)
{
    /* 清理资源 */
    return 0;
}

/* 模块操作接口 */
static module_ops_t wifi_ops = {
    .init = wifi_init,
    .deinit = wifi_deinit,
    .msg_handler = wifi_msg_handler,
    .name = "WiFi Module",
    .id = MODULE_ID_WIFI,
};

/* 模块实例 */
static module_t wifi_module = {
    .ops = &wifi_ops,
    .active = RT_FALSE,
    .thread = RT_NULL,
    .priv_data = RT_NULL,
};
```

### 2. 注册模块

```c
/* 在应用初始化时注册模块 */
int app_init(void)
{
    module_register(&wifi_module);
    module_register(&xiaozhi_module);
    module_register(&ui_module);
    /* ... */
    return 0;
}
```

### 3. 发送消息

```c
/* 方式1：使用完整消息结构 */
app_message_t msg;
msg.header.type = MSG_TYPE_WIFI_CONNECTED;
msg.header.src_module = MODULE_ID_WIFI;
msg.header.dst_module = MODULE_ID_UI;  /* 或0表示广播 */
msg.header.data_len = sizeof(msg_wifi_data_t);
msg.data.wifi.state = WIFI_STATE_CONNECTED;
module_send_msg(&msg);

/* 方式2：使用简化接口 */
module_send_msg_simple(MSG_TYPE_WIFI_CONNECTED, 
                       MODULE_ID_WIFI, 
                       MODULE_ID_UI,
                       &wifi_data,
                       sizeof(wifi_data));
```

### 4. 接收消息

```c
/* 在模块线程中接收消息 */
void wifi_thread(void *parameter)
{
    app_message_t msg;
    
    while (1) {
        /* 阻塞接收消息 */
        if (msg_bus_receive(MODULE_ID_WIFI, &msg, RT_WAITING_FOREVER) == RT_EOK) {
            /* 处理消息 */
            wifi_module.ops->msg_handler(&msg);
        }
    }
}
```

## 新增模块示例

以添加4G模块为例：

### 1. 在msg_def.h中添加消息类型

```c
/* 4G模块消息 (0x20 ~ 0x2F) */
MSG_TYPE_4G_CONNECTED        = 0x20,
MSG_TYPE_4G_DISCONNECTED     = 0x21,
```

### 2. 在module.h中添加模块ID

```c
MODULE_ID_4G = 0x06,
```

### 3. 实现模块

```c
/* 4G模块实现 */
static module_ops_t g4_ops = {
    .init = g4_init,
    .deinit = g4_deinit,
    .msg_handler = g4_msg_handler,
    .name = "4G Module",
    .id = MODULE_ID_4G,
};

static module_t g4_module = {
    .ops = &g4_ops,
    .active = RT_FALSE,
    .thread = RT_NULL,
    .priv_data = RT_NULL,
};
```

### 4. 注册模块

```c
module_register(&g4_module);
```

**完成！** 无需修改其他模块代码。

## 优势

1. **低耦合**：模块间不直接依赖，只依赖消息总线
2. **易扩展**：新增模块不影响现有模块
3. **易测试**：模块可以独立测试
4. **易维护**：清晰的架构，便于理解和维护
5. **标准化**：统一的接口规范，代码风格一致

## 注意事项

1. 消息结构体大小限制为 `MSG_TOTAL_SIZE`（约300字节）
2. 最大模块数限制为 `MAX_MODULES`（16个）
3. 每个模块的消息队列大小为 `MSG_QUEUE_SIZE`（128条）
4. 消息处理函数应该快速返回，避免阻塞

## 目录结构

```
framework/
├── core/              # 核心框架
│   ├── msg_def.h      # 消息定义
│   ├── module.h       # 模块接口
│   ├── module.c       # 模块管理实现
│   ├── msg_bus.h      # 消息总线接口
│   └── msg_bus.c      # 消息总线实现
├── modules/           # 功能模块（待实现）
│   ├── wifi_module.c
│   ├── xiaozhi_module.c
│   ├── ui_module.c
│   └── audio_module.c
└── README.md          # 本文档
```

