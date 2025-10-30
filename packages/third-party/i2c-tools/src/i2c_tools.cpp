/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019-2023 Wu Han
 */

#include "i2c_utils.h"

static void i2c(int argc,char *argv[])
{
    if(argc > 2)
    {
        #ifdef I2C_TOOLS_USE_SW_I2C
            if( i2c_init(strtonum(argv[ARG_SDA_POS]), strtonum(argv[ARG_SCL_POS])) )
            {
                rt_kprintf("[i2c] failed to find bus with sda=%s scl=%s\n", argv[ARG_SDA_POS], argv[ARG_SCL_POS]);
                return;
            }
        #else
            if(i2c_init(argv[ARG_BUS_NAME_POS]))
            {
                rt_kprintf("[i2c] failed to find bus %s\n", argv[ARG_BUS_NAME_POS]);
                return;
            }
        #endif

        if(!strcmp(argv[ARG_CMD_POS], "scan"))
        {
            rt_uint32_t start_addr = 0x00;
            rt_uint32_t stop_addr  = 0x80;
        #ifdef I2C_TOOLS_USE_SW_I2C
            if(argc >= 5)
            {
                start_addr = strtonum(argv[4]);
                if(argc > 5)
                {
                    stop_addr = strtonum(argv[5]);
                }
            }
        #else
            if(argc >= 4)
            {
                start_addr = strtonum(argv[3]);
                if(argc > 4)
                {
                    stop_addr = strtonum(argv[4]);
                }
            }
        #endif
            if (start_addr >= 0x80 || stop_addr > 0x80) {
                rt_kprintf("[i2c] The addresses only range from 0x00 to 0x7F\n");
                return;
            }
            if (start_addr >= stop_addr) {
                rt_kprintf("[i2c] The stop address should be higher than the start address\n");
                return;
            }
            i2c_scan(start_addr, stop_addr);
            return;
        }

        if(argc < 5)
        {
            i2c_help();
            return;
        }

        if(!strncmp(argv[ARG_CMD_POS], "read", 4))
        {
            rt_uint8_t buffer[I2C_TOOLS_BUFFER_SIZE] = {0};
            rt_uint16_t reg = strtonum(argv[ARG_DATA_POS]);
            rt_uint8_t len = 1, ret = 0;
            rt_bool_t  bit16_mode = false;

            if (!strncmp(argv[ARG_CMD_POS], "read16", 6))
                bit16_mode = true;

            if (argc == ARG_READ_MAX)
            {
                len = atoi(argv[ARG_READ_MAX - 1]);
            }
            if (len > I2C_TOOLS_BUFFER_SIZE)
            {
                rt_kprintf("The length %d is too long than the max %d\n",
                           len, I2C_TOOLS_BUFFER_SIZE);
                len = I2C_TOOLS_BUFFER_SIZE;
            }

            if (bit16_mode)
                ret = i2c_read16(strtonum(argv[ARG_ADDR_POS]), reg, buffer, len);
            else
                ret = i2c_read(strtonum(argv[ARG_ADDR_POS]), reg, buffer, len);
            if (ret == len)
            {
                for (rt_uint8_t i = 0; i < len; i++, reg++)
                {
                    if (i && (i % 16 == 0))
                        rt_kprintf("\n");
                    if (i % 16 == 0)
                        rt_kprintf("0x%02X: ", reg);
                    else if (i % 8 == 0)
                        rt_kprintf("   ");

                    rt_kprintf("%02X ", buffer[i]);
                }
                rt_kprintf("\n");
            }
            else
            {
                #ifdef I2C_TOOLS_USE_SW_I2C
                    rt_kprintf("[i2c] read from bus failed with sda=%s scl=%s\n", argv[ARG_SDA_POS], argv[ARG_SCL_POS]);
                #else
                    rt_kprintf("[i2c] read from %s failed\n", argv[ARG_BUS_NAME_POS]);
                #endif
            }
        }

        else if(!strncmp(argv[ARG_CMD_POS], "write", 5))
        {
            unsigned char buffer[I2C_TOOLS_BUFFER_SIZE] = {0};
            rt_bool_t bit16_mode = false, ret = false;
            int data_start = 0;

            if (!strncmp(argv[ARG_CMD_POS], "write16", 7))
                bit16_mode = true;

            if (bit16_mode) {
                rt_uint16_t reg = strtonum(argv[ARG_DATA_POS]);
                buffer[0] = reg >> 8;
                buffer[1] = reg & 0xFF;
                data_start = 1;
            } else {
                buffer[0] = strtonum(argv[ARG_DATA_POS]);
            }

            for(int i=1; i<(argc-ARG_DATA_POS); i++)
            {
                buffer[data_start + i] = strtonum(argv[i+ARG_DATA_POS]);
            }

            if (bit16_mode)
                ret = i2c_write16(strtonum(argv[ARG_DATA_POS-1]), buffer, argc - ARG_DATA_POS + 1);
            else
                ret = i2c_write(strtonum(argv[ARG_DATA_POS-1]), buffer, argc - ARG_DATA_POS);
            if (!ret)
            {
                #ifdef I2C_TOOLS_USE_SW_I2C
                    rt_kprintf("[i2c] write to bus failed with sda=%s scl=%s\n", argv[ARG_SDA_POS], argv[ARG_SCL_POS]);
                #else
                    rt_kprintf("[i2c] write to %s failed\n", argv[ARG_BUS_NAME_POS]);
                #endif
            }
        }
    }
    else
    {
        i2c_help();
    }
}
MSH_CMD_EXPORT(i2c, i2c tools);
