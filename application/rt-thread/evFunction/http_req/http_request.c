#include <rtthread.h>
#include <rtdevice.h>
#include "rtdbg.h"
#include "aic_core.h"

#include "webclient.h"
#include "sys/socket.h"
#include "cJSON.h"
// #include "finsh.h"

#define LOG_TAG "HTTP_DEMO"

#define GET_HEADER_BUF_SIZE        1024        //头部大小
#define GET_READ_BUF_SIZE       2048        //响应缓冲区大小
#define GET_URL_MAX_LEN         512         //网址最大长度
#define GET_URI "http://api.seniverse.com/v3/weather/now.json?key=STMUWUV5jyKzTL4nm&location=dongguan"

uint8_t *http_get_request(uint8_t *url) {
    struct webclient_session *session = NULL;
    rt_uint8_t *read_buf = rt_malloc(GET_READ_BUF_SIZE);
    rt_uint8_t *get_url = rt_malloc(GET_URL_MAX_LEN);
    int response_status;
    int content_length = -1;
    int content_pos = 0;
    int bytes_read = 0;

    if (read_buf == NULL) {
        LOG_E("read_buf分配内存失败");
    };

    rt_snprintf(get_url, GET_URL_MAX_LEN, "%s", url);
    LOG_I("url:%s", get_url);

    //创建 web客户端回话
    session = webclient_session_create(GET_HEADER_BUF_SIZE);
    if (session == NULL) {
        LOG_E("session创建失败\n");
    }

    response_status = webclient_get(session, url);
    if (response_status != 200) {
        LOG_E("web客户端 Get 请求失败,状态码:%d", response_status);
        goto _exit;
    }

    content_length = webclient_content_length_get(session);

    while (content_pos < content_length) {

        bytes_read = webclient_read(session, read_buf, GET_READ_BUF_SIZE);
        LOG_I("Response data size:%d", bytes_read);
        if (bytes_read <= 0) {
            break;
        }
        content_pos += bytes_read;
    }
    //关闭 web客户端会话
    if (session) {
        webclient_close(session);
    }

_exit:
    if (session) {
        webclient_close(session);
    }
    if (read_buf) {
        free(read_buf);
    }

    return read_buf;
}

void http_test(void) {
    LOG_W("http demo");
    uint8_t *response = http_get_request(GET_URI);
    for (int i = 0;i < strlen(response);i++) {
        rt_kprintf("%c", response[i]);
    }
    rt_kprintf("\n");

}

MSH_CMD_EXPORT(http_test, http_demo_test);
