/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-07-10        the first version
 */

#include "gsl3676.h"
#include "touch_common.h"

#define DBG_TAG "gsl3676"
#define DBG_LVL DBG_LOG

#if GSL3676_USE_5_POINT
unsigned int gsl_config_data_id[] =
{
    0xc9009c,
    0x200,
    0,0,
    0,
    0,0,0,
    0,0,0,0,0,0,0,0xb06317b,


    0x40100d0e,0x5,0x7001c,0x7001c,0x1e00780,0,0x5100,0x8e00,
    0,0x320014,0,0,0,0,0,0,
    0x1,0x1000,0x400,0x10000000,0x10000000,0,0,0,
    0x1b6db688,0x100,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0x804000,0x90040,0x90001,0,0,0,
    0,0,0,0x14012c,0xa003c,0xa0078,0x400,0x1081,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,

    0,//key_map
    0x3200384,0x64,0x503e8,//0
    0,0,0,//1
    0,0,0,//2
    0,0,0,//3
    0,0,0,//4
    0,0,0,//5
    0,0,0,//6
    0,0,0,//7

    0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,


    0x220,
    0,0,0,0,0,0,0,0,
    0x10203,0x4050607,0x8090a0b,0xc0d0e0f,0x10111213,0x14151617,0x18191a1b,0x1c1d1e1f,
    0x20212223,0x24252627,0x28292a2b,0x2c2d2e2f,0x30313233,0x34353637,0x38393a3b,0x3c3d3e3f,
    0x10203,0x4050607,0x8090a0b,0xc0d0e0f,0x10111213,0x14151617,0x18191a1b,0x1c1d1e1f,
    0x20212223,0x24252627,0x28292a2b,0x2c2d2e2f,0x30313233,0x34353637,0x38393a3b,0x3c3d3e3f,

    0x10203,0x4050607,0x8090a0b,0xc0d0e0f,0x10111213,0x14151617,0x18191a1b,0x1c1d1e1f,
    0x20212223,0x24252627,0x28292a2b,0x2c2d2e2f,0x30313233,0x34353637,0x38393a3b,0x3c3d3e3f,

    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,

    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,

    0x10203,0x4050607,0x8090a0b,0xc0d0e0f,0x10111213,0x14151617,0x18191a1b,0x1c1d1e1f,
    0x20212223,0x24252627,0x28292a2b,0x2c2d2e2f,0x30313233,0x34353637,0x38393a3b,0x3c3d3e3f,

    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,


    0,
    0x101,0,0x100,0,
    0x20,0x10,0x8,0x4,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,

    0x4,0,0,0,0,0,0,0,
    0x1c00700,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,
};
#else
unsigned int gsl_config_data_id[] =
{
    0xc9007b,
    0x200,
    0,0,
    0,
    0,0,0,
    0,0,0,0,0,0,0,0xb063178,


    0x40100d0e,0x2,0x7001c,0x7001c,0x1e00780,0,0x5100,0x8e00,
    0,0x320014,0,0,0,0,0,0,
    0x1,0x1000,0x400,0x10000000,0x10000000,0,0,0,
    0x1b6db688,0x100,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0x804000,0x90040,0x90001,0,0,0,
    0,0,0,0x14012c,0xa003c,0xa0078,0x400,0x1081,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,

    0,//key_map
    0x3200384,0x64,0x503e8,//0
    0,0,0,//1
    0,0,0,//2
    0,0,0,//3
    0,0,0,//4
    0,0,0,//5
    0,0,0,//6
    0,0,0,//7

    0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,


    0x220,
    0,0,0,0,0,0,0,0,
    0x10203,0x4050607,0x8090a0b,0xc0d0e0f,0x10111213,0x14151617,0x18191a1b,0x1c1d1e1f,
    0x20212223,0x24252627,0x28292a2b,0x2c2d2e2f,0x30313233,0x34353637,0x38393a3b,0x3c3d3e3f,
    0x10203,0x4050607,0x8090a0b,0xc0d0e0f,0x10111213,0x14151617,0x18191a1b,0x1c1d1e1f,
    0x20212223,0x24252627,0x28292a2b,0x2c2d2e2f,0x30313233,0x34353637,0x38393a3b,0x3c3d3e3f,

    0x10203,0x4050607,0x8090a0b,0xc0d0e0f,0x10111213,0x14151617,0x18191a1b,0x1c1d1e1f,
    0x20212223,0x24252627,0x28292a2b,0x2c2d2e2f,0x30313233,0x34353637,0x38393a3b,0x3c3d3e3f,

    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,

    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,

    0x10203,0x4050607,0x8090a0b,0xc0d0e0f,0x10111213,0x14151617,0x18191a1b,0x1c1d1e1f,
    0x20212223,0x24252627,0x28292a2b,0x2c2d2e2f,0x30313233,0x34353637,0x38393a3b,0x3c3d3e3f,

    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,


    0,
    0x101,0,0x100,0,
    0x20,0x10,0x8,0x4,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,

    0x4,0,0,0,0,0,0,0,
    0x1c00700,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,
};
#endif

static struct rt_i2c_client gsl3676_client;

static int gsl3676_write_reg(struct rt_i2c_client *dev, rt_uint8_t *data, rt_uint8_t len)
{
    struct rt_i2c_msg msgs;

    msgs.addr = dev->client_addr;
    msgs.flags = RT_I2C_WR;
    msgs.buf = data;
    msgs.len = len;
    if (rt_i2c_transfer(dev->bus, &msgs, 1) == 1) {
        return RT_EOK;
    } else {
        rt_kprintf("gsl3676 write reg error\n");
        return -RT_ERROR;
    }
}

static int gsl3676_read_reg(struct rt_i2c_client *dev, rt_uint8_t reg, rt_uint8_t *buf, rt_uint8_t len)
{
    struct rt_i2c_msg msgs[2];
    rt_uint8_t temp_reg;

    temp_reg = reg;

    msgs[0].addr  = dev->client_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = &temp_reg;
    msgs[0].len   = 1;

    msgs[1].addr  = dev->client_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf   = buf;
    msgs[1].len   = len;

    if (rt_i2c_transfer(dev->bus, msgs, 2) == 2) {
        return RT_EOK;
    } else {
        rt_kprintf("gsl3676 read reg error\n");
        return -RT_ERROR;
    }
}

static void gsl3676_clr_reg(void)
{
    rt_uint8_t buf[2];

    buf[0] = GSL3676_RESET_REG;
    buf[1] = 0x88;
    gsl3676_write_reg(&gsl3676_client, buf, 2);
    rt_thread_mdelay(20);

    buf[0] = GSL3676_TOUCH_REG;
    buf[1] = 0x3;
    gsl3676_write_reg(&gsl3676_client, buf, 2);
    rt_thread_mdelay(5);

    buf[0] = GSL3676_CLK_REG;
    buf[1] = 0x4;
    gsl3676_write_reg(&gsl3676_client, buf, 2);
    rt_thread_mdelay(5);

    buf[0] = GSL3676_RESET_REG;
    buf[1] = 0x0;
    gsl3676_write_reg(&gsl3676_client, buf, 2);
    rt_thread_mdelay(20);
}

static void gsl3676_reset_chip(void)
{
    rt_uint8_t buf[5];

    buf[0] = GSL3676_RESET_REG;
    buf[1] = 0x88;
    gsl3676_write_reg(&gsl3676_client, buf, 2);
    rt_thread_mdelay(10);

    buf[0] = GSL3676_CLK_REG;
    buf[1] = 0x4;
    gsl3676_write_reg(&gsl3676_client, buf, 2);
    rt_thread_mdelay(10);

    buf[0] = GSL3676_POWER_OFF_DETECT_REG;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
    buf[4] = 0;
    gsl3676_write_reg(&gsl3676_client, buf, 5);
    rt_thread_mdelay(10);
}

static void gsl3676_startup_chip(void)
{
    rt_uint8_t buf[2];

    gsl_DataInit(gsl_config_data_id);
    buf[0] = GSL3676_RESET_REG;
    buf[1] = 0x0;
    gsl3676_write_reg(&gsl3676_client, buf, 2);
    rt_thread_mdelay(10);
}

static int gsl3676_check_mem_data(void)
{
    int ret = 0;
    rt_uint8_t buf[4];

    rt_thread_mdelay(30);
    gsl3676_read_reg(&gsl3676_client, GSL3676_CHIP_INSPECTION_REG, buf, 4);
    if (buf[0] != GSL3676_MEM_DATA || buf[1] != GSL3676_MEM_DATA \
            || buf[2] != GSL3676_MEM_DATA || buf[3] != GSL3676_MEM_DATA) {
                LOG_D("gsl3676 chip status is abnormal");
                return -1;
            }

    return ret;
}

static void gsl3676_load_fw(void)
{
    rt_uint8_t buf[5];
    rt_uint8_t reg;
    rt_uint32_t source_line = 0;
    rt_uint32_t source_len = sizeof(GSL_FW) / sizeof(struct fw_data);

    for (source_line = 0; source_line < source_len; source_line++) {
        reg = GSL_FW[source_line].offset;
        buf[0] = reg;
        buf[1] = (char)(GSL_FW[source_line].val & 0x000000ff);
        buf[2] = (char)((GSL_FW[source_line].val & 0x0000ff00) >> 8);
        buf[3] = (char)((GSL_FW[source_line].val & 0x00ff0000) >> 16);
        buf[4] = (char)((GSL_FW[source_line].val & 0xff000000) >> 24);

        gsl3676_write_reg(&gsl3676_client, buf, 5);
    }
}

static void gsl3676_init(rt_base_t pin)
{
    int ret = 0;

    rt_pin_mode(pin, PIN_MODE_OUTPUT);
    rt_pin_write(pin, 0);
    rt_thread_mdelay(20);
    rt_pin_write(pin, 1);
    rt_thread_mdelay(20);
    gsl3676_clr_reg();
    gsl3676_reset_chip();
    gsl3676_startup_chip();

    ret = gsl3676_check_mem_data();
    if (ret) {
        rt_pin_write(pin, 0);
        rt_thread_mdelay(20);
        rt_pin_write(pin, 1);
        rt_thread_mdelay(20);
        gsl3676_clr_reg();
        gsl3676_reset_chip();
        gsl3676_load_fw();
        gsl3676_startup_chip();
    }
}

#if GSL3676_USE_5_POINT
static int16_t pre_x[GSL3676_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static int16_t pre_y[GSL3676_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
#else
static int16_t pre_x[GSL3676_MAX_TOUCH] = { -1, -1 };
static int16_t pre_y[GSL3676_MAX_TOUCH] = { -1, -1 };
#endif

static rt_uint32_t s_tp_dowm[GSL3676_MAX_TOUCH] = {0};
static struct rt_touch_data *read_data = RT_NULL;

static void gsl3676_touch_up(void *buf, int8_t id)
{
    read_data = (struct rt_touch_data *)buf;

    if (s_tp_dowm[id] == 1) {
        s_tp_dowm[id] = 0;
        read_data[id].event = RT_TOUCH_EVENT_UP;
    } else {
        read_data[id].event = RT_TOUCH_EVENT_NONE;
    }

    read_data[id].timestamp = rt_touch_get_ts();
    read_data[id].x_coordinate = pre_x[id];
    read_data[id].y_coordinate = pre_y[id];
    read_data[id].track_id = id;

    pre_x[id] = -1; /* last point is none */
    pre_y[id] = -1;
}

static void gsl3676_touch_down(void *buf, int8_t id, int16_t x, int16_t y)
{
    read_data = (struct rt_touch_data *)buf;

    if (s_tp_dowm[id] == 1) {
        read_data[id].event = RT_TOUCH_EVENT_MOVE;
    } else {
        read_data[id].event = RT_TOUCH_EVENT_DOWN;
        s_tp_dowm[id] = 1;
    }

    read_data[id].timestamp = rt_touch_get_ts();
    read_data[id].x_coordinate = x;
    read_data[id].y_coordinate = y;
    read_data[id].track_id = id;

    pre_x[id] = x; /* save last point */
    pre_y[id] = y;
}


static rt_size_t gsl3676_read_points(struct rt_touch_device *touch, void *buf, rt_size_t read_num)
{
    rt_uint8_t touch_num = 0;
    rt_uint8_t read_buf[44] = {0};
    rt_uint8_t read_index = 0;
    int8_t read_id = 0;
    int16_t input_x = 0;
    int16_t input_y = 0;
    static rt_uint8_t pre_touch = 0;
    unsigned int temp_a, temp_b, i;
    struct gsl_touch_info cinfo = {{0},{0},{0},0};
    static uint32_t pre_id[GSL3676_MAX_TOUCH] = { 0 };
    u32 tmp1;
    u8 cmd_buf[5] = {0};

    rt_memset(buf, 0, sizeof(struct rt_touch_data) * read_num);

    gsl3676_read_reg(&gsl3676_client, GSL3676_TOUCH_REG, read_buf, 44);
    /* get point num */
    touch_num = read_buf[0] & 0xf;
    cinfo.finger_num = touch_num;

    for(i = 0; i < (touch_num < MAX_CONTACTS ? touch_num : MAX_CONTACTS); i ++)
    {
        temp_a = read_buf[(i + 1) * 4 + 3] & 0x0f;
        temp_b = read_buf[(i + 1) * 4 + 2];
        cinfo.x[i] = temp_a << 8 |temp_b;
        temp_a = read_buf[(i + 1) * 4 + 1];
        temp_b = read_buf[(i + 1) * 4 + 0];
        cinfo.y[i] = temp_a << 8 |temp_b;
        cinfo.id[i] = ((read_buf[7 + i * 4] & 0xf0) >> 4);
    }

    cinfo.finger_num = read_buf[3] << 24 | read_buf[2] << 16 | read_buf[1] << 8 | read_buf[0];
    gsl_alg_id_main(&cinfo);
    tmp1 = gsl_mask_tiaoping();

    if (tmp1 > 0 && tmp1 < 0xffffffff) {
        cmd_buf[0] = 0xf0;
        cmd_buf[1] = 0xa;
        cmd_buf[2] = 0;
        cmd_buf[3] = 0;
        cmd_buf[4] = 0;
        gsl3676_write_reg(&gsl3676_client, cmd_buf, 5);

        cmd_buf[0] = 0x80;
        cmd_buf[1] = (u8)(tmp1 & 0xff);
        cmd_buf[2] = (u8)((tmp1 >> 8) & 0xff);
        cmd_buf[3] = (u8)((tmp1 >> 16) & 0xff);
        cmd_buf[4] = (u8)((tmp1 >> 24) & 0xff);
        gsl3676_write_reg(&gsl3676_client, cmd_buf, 5);
    }

    touch_num = cinfo.finger_num;

    if (touch_num  > GSL3676_MAX_TOUCH) {
        touch_num = 0;
    }

    for (int recombine_id = 0; recombine_id < touch_num; recombine_id++)
        cinfo.id[recombine_id] = recombine_id;

    /* point up */
    if (pre_touch > touch_num) {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            rt_uint8_t j;
            /* this time touch num */
            for (j = 0; j < touch_num; j++) {
                read_id = cinfo.id[read_index];

                if (pre_id[read_index] == read_id)      /* this id is not free */
                    break;

                if (j >= (touch_num -1)) {
                    rt_uint8_t up_id;
                    up_id = pre_id[read_index];
                    gsl3676_touch_up(buf, up_id);
                }
            }
        }
    }

    /* point down */
    if (touch_num) {
        for (read_index = 0; read_index < touch_num; read_index++) {

            read_id = cinfo.id[read_index];
            pre_id[read_index] = read_id;

            input_y = cinfo.y[read_index];
            input_x = cinfo.x[read_index];

            aic_touch_flip(&input_x, &input_y);
            aic_touch_rotate(&input_x, &input_y);
            aic_touch_scale(&input_x, &input_y);
            if (!aic_touch_crop(&input_x, &input_y))
                continue;

            gsl3676_touch_down(buf, read_id, input_x, input_y);
        }
    } else if (pre_touch) {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            gsl3676_touch_up(buf, pre_id[read_index]);
        }
    }

    pre_touch = touch_num;

    return read_num;
}

static rt_err_t gsl3676_control(struct rt_touch_device *touch, int cmd, void *arg)
{
    struct rt_touch_info *info;

    switch (cmd) {
    case RT_TOUCH_CTRL_GET_INFO:
        info = (struct rt_touch_info *)arg;
        if (info == RT_NULL) {
            return -RT_EINVAL;
        }

        info->point_num = touch->info.point_num;
        info->range_x = touch->info.range_x;
        info->range_y = touch->info.range_y;
        info->type = touch->info.type;
        info->vendor = touch->info.vendor;
        break;
    case RT_TOUCH_CTRL_SET_MODE:
    case RT_TOUCH_CTRL_SET_X_RANGE:
    case RT_TOUCH_CTRL_SET_Y_RANGE:
    case RT_TOUCH_CTRL_SET_X_TO_Y:
    case RT_TOUCH_CTRL_DISABLE_INT:
    case RT_TOUCH_CTRL_ENABLE_INT:
    case RT_TOUCH_CTRL_GET_ID:
    default:
        break;
    }

    return RT_EOK;
}

const struct rt_touch_ops gsl3676_ops = {
    .touch_readpoint = gsl3676_read_points,
    .touch_control = gsl3676_control,
};

struct rt_touch_info gsl3676_info = {
    RT_TOUCH_TYPE_CAPACITANCE,
    RT_TOUCH_VENDOR_UNKNOWN,
    10,
    (rt_int32_t)AIC_TOUCH_PANEL_GSL3676_X_RANGE,
    (rt_int32_t)AIC_TOUCH_PANEL_GSL3676_Y_RANGE,
};

int gsl3676_hw_init(const char *name, struct rt_touch_config *cfg)
{
    struct rt_touch_device *touch_device = RT_NULL;

    touch_device = (struct rt_touch_device *)rt_malloc(sizeof(struct rt_touch_device));
    if (touch_device == RT_NULL) {
        LOG_E("touch device malloc fail");
        return -RT_ERROR;
    }
    rt_memset((void *)touch_device, 0, sizeof(struct rt_touch_device));
    gsl3676_client.bus =
        (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);
    if (gsl3676_client.bus == RT_NULL) {
        LOG_E("Can't find %s device", cfg->dev_name);
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)gsl3676_client.bus, RT_DEVICE_FLAG_RDWR) != RT_EOK) {
        LOG_E("open %s device failed", cfg->dev_name);
        return -RT_ERROR;
    }

    gsl3676_client.client_addr = GSL3676_SLAVE_ADDR;

    gsl3676_init(*(rt_uint8_t *)cfg->user_data);

    /* register touch device */
    touch_device->info = gsl3676_info;
    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &gsl3676_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        LOG_E("touch device gsl3676 init failed !!!");
        return -RT_ERROR;
    }

    LOG_I("touch device gsl3676 init success");
    return RT_EOK;
}

static int gsl3676_gpio_cfg()
{
    unsigned int g, p;
    long pin;

    // RST
    pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);
    hal_gpio_direction_input(g, p);

    // INT
    pin = drv_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);
    hal_gpio_direction_input(g, p);
    hal_gpio_set_irq_mode(g, p, 0);

    return 0;
}

void gsl3676_power_down(void)
{
    rt_uint8_t rst_pin;

    gsl3676_gpio_cfg();
    rst_pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    /* power down the reset pin */
    rt_pin_mode(rst_pin, PIN_MODE_OUTPUT);
    rt_pin_write(rst_pin, 0);
}

void gsl3676_power_up(void)
{
    rt_uint8_t rst_pin;
    int ret = 0;

    gsl3676_gpio_cfg();
    rst_pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN);

    /* power up the reset pin */
    rt_pin_mode(rst_pin, PIN_MODE_OUTPUT);
    rt_pin_write(rst_pin, 1);
    rt_thread_mdelay(20);

    /* reset chip */
    gsl3676_reset_chip();
    /* restart chip */
    gsl3676_startup_chip();
    rt_thread_mdelay(20);
    /* check the state of chip */
    ret = gsl3676_check_mem_data();
    if (ret) {
        rt_pin_write(rst_pin, 0);
        rt_thread_mdelay(20);
        rt_pin_write(rst_pin, 1);
        rt_thread_mdelay(20);
        gsl3676_clr_reg();
        gsl3676_reset_chip();
        gsl3676_load_fw();
        gsl3676_startup_chip();
    }
}

static int rt_hw_gsl3676_init(void)
{
    struct rt_touch_config cfg;
    rt_uint8_t rst_pin;

    gsl3676_gpio_cfg();

    rst_pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    cfg.dev_name = AIC_TOUCH_PANEL_I2C_CHAN;
    cfg.irq_pin.pin = drv_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    cfg.irq_pin.mode = PIN_MODE_INPUT;
    cfg.user_data = &rst_pin;

    gsl3676_hw_init("gsl3676", &cfg);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_gsl3676_init);
