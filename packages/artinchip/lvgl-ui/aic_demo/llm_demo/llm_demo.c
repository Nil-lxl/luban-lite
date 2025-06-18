/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "string.h"
#include "llm_demo.h"
#include "rtthread.h"
#include "view/view.h"
#include "./module/thread_pool.h"

#define THREADPOOL_MAX 3
#define WIFI_NAME      NULL
#define WIFI_PASSWORD  NULL
typedef struct _lv_llm_msg
{
    int type;
    bool has_text;
    bool has_image;
    bool has_audio;
    lv_obj_t *obj;
    char *message;
    llm_config_t *llm_cfg;
} lv_llm_msg_t;

static lv_llm_config_t global_llm_config;

static lv_timer_t *ui_resp_timer;
static thread_pool_t *llm_thread_pool;
static rt_mq_t task_req_mq;
static rt_mq_t ui_resp_mq;

static void wifi_config_init(const char *wifi_name, const char *passward);

static void llm_msg_init(void);

static void ui_handle_response(lv_timer_t *t);

static void llm_send_ui_msg_cb(const char *message, void *usr_data);

static void llm_request_handle(void *ptr);
static void wifi_config_handle(void *ptr);

static void llm_screen_event_cb(lv_event_t *e);

extern char *llm_generate_response(llm_config_t *config);

void ui_init(void)
{
    llm_config_load(LLM_DEEPSEEK_V3, NULL);

    lv_obj_t *llm_scr = lv_obj_create(NULL);
    lv_obj_clear_flag(llm_scr, LV_OBJ_FLAG_SCROLLABLE);
    chat_ui_create(llm_scr);
    lv_obj_add_event_cb(llm_scr, llm_screen_event_cb, LV_EVENT_ALL, NULL);
    lv_scr_load(llm_scr);

    llm_msg_init();

    llm_thread_pool = thread_pool_create(THREADPOOL_MAX);
    if (!llm_thread_pool) {
        LV_LOG_ERROR("Failed to create thread pool");
        return;
    }

    wifi_config_init(WIFI_NAME, WIFI_PASSWORD);

    ui_resp_timer = lv_timer_create(ui_handle_response, 50, NULL);
}

int llm_config_load(llm_type_t type, const char *config_file)
{
    LV_UNUSED(config_file);

    typedef struct {
        lv_color_t color;
        char *logo;
        char *url;
        char *api_key;
        char *model;
    } llm_config_entry;

    static llm_config_entry config_table[] = {
        [LLM_DEEPSEEK_DISTALL] = {
            .color = {.blue = 0xfe, .green = 0x6b, .red = 0x4F},
            .logo = LVGL_PATH(deepseek.png),
            .url = "https://api.siliconflow.cn/v1/chat/completions",
            .api_key = "sk-xheizvzayhbxgqvycgfhfteaqbzndofkvlggvmxflyhxeepl",
            .model = "deepseek-ai/DeepSeek-R1-Distill-Qwen-7B"
        },
        [LLM_DEEPSEEK_V3] = {
            .color = {.blue = 0xff, .green = 0xff, .red = 0xff},
            .logo = LVGL_PATH(deepseek.png),
            .url = "https://api.siliconflow.cn/v1/chat/completions",
            .api_key = "sk-xheizvzayhbxgqvycgfhfteaqbzndofkvlggvmxflyhxeepl",
            .model = "Pro/deepseek-ai/DeepSeek-V3"
        },
        [LLM_TONGYI_QWQ32B] = {
            .color = {.blue = 0xed, .green = 0x5c, .red = 0x61},
            .logo = LVGL_PATH(tongyi.png),
            .url = "https://api.siliconflow.cn/v1/chat/completions",
            .api_key = "sk-ttmshmgyiiyhbwoexmkzbtiobjulqpleyoghsjawhhhfhtgx",
            .model = "Qwen/QwQ-32B"
        }
    };

    if (type < 0 || type >= LLM_TYPE_MAX) {
        LV_LOG_WARN("Unsupported LLM type: %d", type);
        return -1;
    }

    const llm_config_entry *config_entry = &config_table[type];
    global_llm_config.color = config_entry->color;
    global_llm_config.logo = config_entry->logo;
    global_llm_config.cfg.url = config_entry->url;
    global_llm_config.cfg.api_key = config_entry->api_key;
    global_llm_config.cfg.model = config_entry->model;
    global_llm_config.cfg.config_file = LVGL_PATH_ORI(config/llm_config.json);
    return 0;
}

int llm_request_submit(const char *user_input, lv_obj_t *obj)
{
    if (!user_input || strlen(user_input) == 0)
        return -1;

    char *prompt = (char *)rt_strdup(user_input);
    if (!prompt)
        return -1;

    llm_config_t *llm_cfg = (llm_config_t *)rt_malloc(sizeof(llm_config_t));
    if (!llm_cfg) {
        rt_free(prompt);
        return -1;
    }
    memcpy(llm_cfg, &global_llm_config, sizeof(llm_config_t));
    llm_cfg->prompt = prompt;
    llm_cfg->user_data = obj;
    llm_cfg->callback = llm_send_ui_msg_cb;
    llm_cfg->callback_para = llm_cfg;

    lv_llm_msg_t msg = {0};
    msg.llm_cfg = llm_cfg;
    msg.message = prompt;

    if (rt_mq_send(task_req_mq, &msg, sizeof(lv_llm_msg_t)) != RT_EOK) {
        rt_free(prompt);
        rt_free(llm_cfg);
        return -1;
    }

    if (thread_pool_add(llm_thread_pool, llm_request_handle, NULL) != 0) {
        rt_free(prompt);
        rt_free(llm_cfg);
        return -1;
    }

    return 0;
}

lv_color_t llm_get_message_style_color(void)
{
    return global_llm_config.color;
}

void * lv_llm_get_message_logo_src(void)
{
    return global_llm_config.logo;
}

void * lv_llm_get_message_user_src(void)
{
    return LVGL_PATH(user.png);
}

lv_color_t llm_get_message_user_color(void)
{
    // return lv_color_hex(0x50cae9);
    return lv_color_hex(0xffffff);
}

void wifi_config_init(const char *wifi_name, const char *password)
{
    if (wifi_name == NULL || password == NULL)
        return;
    static char wifi_info[256] = {0};
    snprintf(wifi_info, sizeof(wifi_info), "%s %s", wifi_name, password);
    thread_pool_add(llm_thread_pool, wifi_config_handle, &wifi_info);
}

static void llm_msg_init(void)
{
    task_req_mq = rt_mq_create("task_req_mq",
                                1024,
                                sizeof(lv_llm_msg_t),
                                RT_IPC_FLAG_FIFO);
    if (!task_req_mq) {
        LV_LOG_ERROR("Failed to create request message queue");
        return;
    }

    ui_resp_mq = rt_mq_create("ui_resp_mq",
                                1024,
                                sizeof(lv_llm_msg_t),
                                RT_IPC_FLAG_FIFO);
    if (!ui_resp_mq) {
        LV_LOG_ERROR("Failed to create response message queue");
        return;
    }
}

static int cmd_exec_rtos(char *command)
{
    extern int msh_exec(char *cmd, rt_size_t length);

    static char cmd[256] = {0};

    strncpy(cmd, command, sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';
    return msh_exec(cmd, strlen(cmd));
}

static void wifi_config_handle(void *ptr)
{
    lv_llm_msg_t msg = {0};

    msg.type = MSG_TYPE_WIFI;
    msg.has_text = true;
    msg.message = rt_strdup(WIFI_START_MSG);
    if (rt_mq_send(ui_resp_mq, &msg, sizeof(lv_llm_msg_t)) != RT_EOK) {
        return;
    }

    msg.message = rt_strdup(WIFI_CONNECTING_MSG);
    if (rt_mq_send(ui_resp_mq, &msg, sizeof(lv_llm_msg_t)) != RT_EOK) {
        return;
    }

    rt_thread_mdelay(1000);
    char wifi_info[256] = {0};
    snprintf(wifi_info, sizeof(wifi_info), "wlan wifi_connect %s", (char *)ptr);
    cmd_exec_rtos("wlan wifi_on");

    int status = cmd_exec_rtos(wifi_info);
    if (status == 47)
        msg.message = rt_strdup(WIFI_CONNECTED_MSG);
    else
        msg.message = rt_strdup(WIFI_ERROR_MSG);
    if (rt_mq_send(ui_resp_mq, &msg, sizeof(lv_llm_msg_t)) != RT_EOK) {
        return;
    }
}

static void llm_screen_event_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);

    if (code == LV_EVENT_SCREEN_UNLOAD_START) {
        lv_timer_delete(ui_resp_timer);
        thread_pool_destroy(llm_thread_pool);
        rt_mq_delete(task_req_mq);
        rt_mq_delete(ui_resp_mq);
    }
    if (code == LV_EVENT_DELETE) {;}
    if (code == LV_EVENT_SCREEN_LOADED) {;}
}

static void ui_handle_response(lv_timer_t *t)
{
    lv_llm_msg_t msg;

    if(rt_mq_recv(ui_resp_mq, &msg, sizeof(lv_llm_msg_t), 0) == RT_EOK)
    {
        if (msg.has_text && msg.type == MSG_TYPE_LLM_TEXT) {
            chat_ui_message_set_text(msg.obj, msg.message);
        } else if (msg.has_text && msg.type == MSG_TYPE_WIFI) {
            handle_ui_message(msg.message);
        }

        if (msg.message) {
            rt_free(msg.message);
        }
    }
}

static void llm_send_ui_msg_cb(const char *message, void *callback_para)
{
    lv_llm_msg_t msg;
    llm_config_t *llm_cfg;

    llm_cfg = (llm_config_t *)callback_para;
    char *new_message = (char *)rt_strdup(message);
    if (!new_message)
        return;

    msg.type = MSG_TYPE_LLM_TEXT;
    msg.has_text = true;
    msg.obj = llm_cfg->user_data;
    msg.message = new_message;

    rt_mq_send(ui_resp_mq, &msg, sizeof(lv_llm_msg_t));
}

static void llm_request_handle(void *ptr)
{
    lv_llm_msg_t msg;
    char *response;

    /* create a thread to handle */
    if (rt_mq_recv(task_req_mq, &msg, sizeof(msg), RT_WAITING_FOREVER) == RT_EOK) {
        response = llm_generate_response(msg.llm_cfg);
        if (response) {
            rt_free(response);
        }
        if (msg.message) {
            rt_free(msg.message);
        }
        if (msg.llm_cfg) {
            rt_free(msg.llm_cfg);
        }
    }
}
