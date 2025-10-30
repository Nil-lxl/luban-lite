/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-12-05     Bernard      the first version
 */

/*
 * COPYRIGHT (C) 2012, Shanghai Real Thread
 */

#include <rtdevice.h>
#include "aic_common.h"

#ifdef RT_USING_MTD_NAND

/**
 * RT-Thread Generic Device Interface
 */
static rt_err_t _mtd_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t _mtd_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t _mtd_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_size_t _mtd_read(rt_device_t dev,
                           rt_off_t    pos,
                           void       *buffer,
                           rt_size_t   size)
{
    return size;
}

static rt_size_t _mtd_write(rt_device_t dev,
                            rt_off_t    pos,
                            const void *buffer,
                            rt_size_t   size)
{
    return size;
}

static rt_err_t _mtd_control(rt_device_t dev, int cmd, void *args)
{
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops mtd_nand_ops =
{
    _mtd_init,
    _mtd_open,
    _mtd_close,
    _mtd_read,
    _mtd_write,
    _mtd_control
};
#endif

rt_err_t rt_mtd_nand_register_device(const char                *name,
                                     struct rt_mtd_nand_device *device)
{
    rt_device_t dev;

    dev = RT_DEVICE(device);
    RT_ASSERT(dev != RT_NULL);

    /* set device class and generic device interface */
    dev->type        = RT_Device_Class_MTD;
#ifdef RT_USING_DEVICE_OPS
    dev->ops         = &mtd_nand_ops;
#else
    dev->init        = _mtd_init;
    dev->open        = _mtd_open;
    dev->read        = _mtd_read;
    dev->write       = _mtd_write;
    dev->close       = _mtd_close;
    dev->control     = _mtd_control;
#endif

    dev->rx_indicate = RT_NULL;
    dev->tx_complete = RT_NULL;

    /* register to RT-Thread device system */
    return rt_device_register(dev, name, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE);
}

rt_uint32_t rt_mtd_nand_read_id(struct rt_mtd_nand_device *device)
{
    RT_ASSERT(device->ops->read_id);
    return device->ops->read_id(device);
}

rt_err_t rt_mtd_nand_read(
    struct rt_mtd_nand_device *device,
    rt_off_t page,
    rt_uint8_t *data, rt_uint32_t data_len,
    rt_uint8_t *spare, rt_uint32_t spare_len)
{
    rt_off_t start_page;

    start_page = page + device->block_start * device->pages_per_block;
    RT_ASSERT(device->ops->read_page);
    return device->ops->read_page(device, start_page, data, data_len, spare, spare_len);
}

rt_err_t rt_mtd_nand_read_cont(
    struct rt_mtd_nand_device *device,
    rt_off_t page,
    rt_uint8_t *data, rt_uint32_t data_len)
{
    rt_off_t start_page;

    start_page = page + device->block_start * device->pages_per_block;
    RT_ASSERT(device->ops->read_page);
    return device->ops->continuous_read(device, start_page, data, data_len);
}

rt_err_t rt_mtd_nand_write(
    struct rt_mtd_nand_device *device,
    rt_off_t page,
    const rt_uint8_t *data, rt_uint32_t data_len,
    const rt_uint8_t *spare, rt_uint32_t spare_len)
{
    rt_off_t start_page;

    start_page = page + device->block_start * device->pages_per_block;
    RT_ASSERT(device->ops->write_page);
    return device->ops->write_page(device, start_page, data, data_len, spare, spare_len);
}

rt_err_t rt_mtd_nand_move_page(struct rt_mtd_nand_device *device,
        rt_off_t src_page, rt_off_t dst_page)
{
    rt_off_t start_src, start_dst;

    start_src = src_page + device->block_start * device->pages_per_block;
    start_dst = dst_page + device->block_start * device->pages_per_block;
    RT_ASSERT(device->ops->move_page);
    return device->ops->move_page(device, start_src, start_dst);
}

rt_err_t rt_mtd_nand_erase_block(struct rt_mtd_nand_device *device, rt_uint32_t block)
{
    RT_ASSERT(device->ops->erase_block);
    return device->ops->erase_block(device, device->block_start + block);
}

rt_err_t rt_mtd_nand_check_block(struct rt_mtd_nand_device *device, rt_uint32_t block)
{
    if (device->ops->check_block)
    {
        return device->ops->check_block(device, device->block_start + block);
    }
    else
    {
        return -RT_ENOSYS;
    }
}

rt_err_t rt_mtd_nand_mark_badblock(struct rt_mtd_nand_device *device, rt_uint32_t block)
{
    if (device->ops->mark_badblock)
    {
        return device->ops->mark_badblock(device, device->block_start + block);
    }
    else
    {
        return -RT_ENOSYS;
    }
}

rt_err_t rt_mtd_nand_map_user(
    struct rt_mtd_nand_device *device,
    rt_uint8_t *oobbuf, rt_uint8_t *data,
    rt_base_t start, rt_base_t nbytes)
{
    RT_ASSERT(device->ops->map_user);
    return device->ops->map_user(device, oobbuf, data, start, nbytes);
}

rt_err_t rt_mtd_nand_unmap_user(
    struct rt_mtd_nand_device *device,
    rt_uint8_t *dst, rt_uint8_t *src,
    rt_base_t start, rt_base_t nbytes)
{
    RT_ASSERT(device->ops->map_user);
    return device->ops->unmap_user(device, dst, src, start, nbytes);
}

#endif /* RT_USING_MTD_NAND */
