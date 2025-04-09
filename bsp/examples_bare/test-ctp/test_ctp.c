/*
 * Copyright (c) 2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../../artinchip/drv_bare/touch/gt911/gt911.h"
#include <console.h>

static int test_ctp_example(int argc, char *argv[])
{
    static struct touch_data *read_data;
    int ret = 0;
    int point_num = 0;
    struct touch *touch = NULL;
    void *id;

    touch = (struct touch *)aicos_malloc(MEM_CMA, sizeof(struct touch));
    if (touch == NULL) {
        hal_log_err("touch malloc fail\n");
        return TOUCH_ERROR;
    }
    memset(touch, 0, sizeof(struct touch));

    read_data = (struct touch_data *)aicos_malloc(MEM_CMA, sizeof(struct touch_data) * 5);
    if (read_data == NULL) {
        printf("read_data malloc fail\n");
        return -1;
    }

    touch->name = "gt911";
    ret = gt911_init(touch);
    if (ret == TOUCH_ERROR) {
        printf("gt911 init fail\n");
        return ret;
    }

    id = aicos_malloc(MEM_DEFAULT, sizeof(uint8_t) * 8);
    touch_control(touch, TOUCH_CTRL_GET_ID, id);

    uint8_t *read_id = (uint8_t *)id;
    printf("id = GT%d%d%d \n", read_id[0] - '0', read_id[1] - '0', read_id[2] - '0');
    aicos_free(MEM_DEFAULT, id);

    while (1) {
        point_num = touch_readpoint(touch, read_data, 5);
        if (point_num > 0) {
            for (uint8_t i = 0; i < 5; i++) {
                if (read_data[i].event == TOUCH_EVENT_UP ||
                    read_data[i].event == TOUCH_EVENT_DOWN ||
                    read_data[i].event == TOUCH_EVENT_MOVE) {
                        printf("[%d] %d %d\n", read_data[i].event,
                                read_data[i].x_coordinate, read_data[i].y_coordinate);
                }
            }
        }
        aicos_mdelay(10);
    }

    return ret;
}
CONSOLE_CMD(test_ctp, test_ctp_example,  "ctp test example");
