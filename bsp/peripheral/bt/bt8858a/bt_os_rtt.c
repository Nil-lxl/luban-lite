#include "bt_config.h"
#include "bt_api.h"
#include "bt_os.h"
#include "bt_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#if RTT

static rt_timer_t   			bt_receivecmd_timer;
static rt_timer_t   			bt_decodecmd_timer;

static rt_thread_t    	uart_recive_task_id;
static rt_thread_t   	uart_decode_task_id;


/**************************bt uart******************************
以下函数是系统层参考函数，需要用户端实现,
uart通讯默认波特率115200，8位数据位，1位停止位，无校验位
**************************bt uart******************************/
//__u32  	COM_UART_BAUD = 115200;
//bt uart operate


#if openmode
static rt_device_t g_fp_uart;

#define UART_NAME       "uart4"

__s32 rtt_com_uart_init(void)
{
	__s32 ret;
    char uart_name[16] = UART_NAME;

    g_fp_uart = rt_device_find(uart_name);
    if (!g_fp_uart)
    {
        printf("find %s failed!\n", uart_name);
        return EPDK_FAIL;
    }
    ret = rt_device_open(g_fp_uart, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    if (ret != EPDK_OK)
    {
        printf("open %s failed : %d !\n", uart_name, ret);
        return EPDK_FAIL;
    }

	bt_msleep(10);
	__wrn("UART open success");

	return EPDK_OK;
}

__s32 rtt_com_uart_deinit(void)
{
	if(g_fp_uart > 0)
	{
		__msg("close UART handle");
		rt_device_close(g_fp_uart);
		g_fp_uart = NULL;
	}

	bt_msleep(10);
	__msg("com_uart_deinit end\n");
	return EPDK_OK;
}

__s32 rtt_com_uart_write(char* pbuf, __s32 size)
{
	__s32 ret = 0;
	//__msg("com_uart_write start:%s,len%d\n",pbuf,size);
	if(g_fp_uart < 0)
	{
		__msg("g_fp_uart handle is NULL\n");
		return EPDK_FAIL;
	}
	//__msg("com_uart_write 1\n");

	while(size > 0)
	{
		//__msg("com_uart_write 2\n");
		ret = rt_device_write(g_fp_uart, 0, pbuf, (rt_size_t)size);

		if(ret > 0)
		{

			size =- ret;
			pbuf += ret;
			//__msg("has write =%d\n", ret);
		}
	}
	//__msg("com_uart_write end:%s\n",pbuf);
	return EPDK_OK;
}

__s32 rtt_com_uart_read(char* pbuf, __s32 buf_size, __s32* size)
{
	__s32 ret = 0;
	__s32 i=0;
	__s32 to_read_size;
	//__msg("com_uart_read start:\n");
	if(g_fp_uart < 0)
	{
		__wrn("uart read: g_fp_uart is<0\n");
		return EPDK_FAIL;
	}
	if(NULL == size)
	{
		__wrn("uart read: invalid para size\n");
		return EPDK_FAIL;
	}
	to_read_size = buf_size;
	memset(pbuf, 0, buf_size);
	ret = rt_device_read(g_fp_uart,-1, pbuf, to_read_size);
	if(ret>0)
	{
		__wrn("====bt string====\n");
		for(i=0;i<ret;i++)
		{
			__wrn("=%x=",pbuf[i]);
		}
		__wrn("\n====string voer====\n");
	}
	//__msg("======read ret%d,==%s======\n",ret,pbuf);
	*size = ret;
	//__msg("---------com_uart_read end\n");
	return EPDK_OK;
}

__s32 rtt_com_uart_flush(void)
{
	__s32 ret = 0;
	if(g_fp_uart >= 0)
	{
	  //  ret = eLIBs_fioctrl(g_fp_uart, UART_CMD_FLUSH, 0, 0);
	    if(ret != EPDK_OK)
		{
		//	__msg("UART FLUSH fail");
		//	return EPDK_FAIL;
		}
		//esKRNL_TimeDly(5);
		bt_msleep(5);
	}
	return EPDK_OK;
}

#endif

#if (runmode == 1)//线程运行

void sys_bt_decode_task(void*parg)
{
    while(1)
    {
        sys_bt_decode_cmd(parg);
    }

}
void sys_bt_receive_task(void*parg)
{
    while(1)
    {
        sys_bt_receive_cmd(parg);
    }

}
__s32 rtt_bt_task_start(void)//task
{
	printf("uart_task_start\n");
    uart_decode_task_id = rt_thread_create("bt_decode_task",
                                   sys_bt_decode_task, RT_NULL,
                                   4096,
                                   6, 10);

    if (uart_decode_task_id != RT_NULL)
        rt_thread_startup(uart_decode_task_id);

    uart_recive_task_id = rt_thread_create("bt_receive_task",
                                   sys_bt_receive_task, RT_NULL,
                                   4096,
                                   7, 10);

    if (uart_recive_task_id != RT_NULL)
        rt_thread_startup(uart_recive_task_id);

	printf("uart_task_start");
    return EPDK_OK;
}

__s32 rtt_bt_task_stop(void)
{
	if(uart_recive_task_id != RT_NULL)
    {
        rt_thread_delete(uart_recive_task_id);
        uart_recive_task_id = RT_NULL;
    }
    if(uart_decode_task_id != RT_NULL)
    {
        rt_thread_delete(uart_decode_task_id);
        uart_decode_task_id = RT_NULL;
    }
	return EPDK_OK;
}

#elif(runmode == 0)//定时器运行

__s32 rtt_bt_task_start(void) timer
{
		printf("rt_timer_create bt_decodecmd_timer\n");
        bt_decodecmd_timer = rt_timer_create("bt_decode_timer",
                                     sys_bt_decode_cmd,
                                     RT_NULL,
                                     20,
                                     RT_TIMER_FLAG_SOFT_TIMER | RT_TIMER_FLAG_PERIODIC);
		if(bt_decodecmd_timer == NULL)
		{
			printf("create bt_decodecmd_timer failed\n");

			goto start_exit;;
		}
		rt_timer_start(bt_decodecmd_timer);

		printf("rt_timer_create bt_receivecmd_timer\n");

        bt_decodecmd_timer = rt_timer_create("bt_receive_timer",
                                     sys_bt_receive_cmd,
                                     RT_NULL,
                                     80,
                                     RT_TIMER_FLAG_SOFT_TIMER | RT_TIMER_FLAG_PERIODIC);
        if(bt_receivecmd_timer == NULL)
		{
			printf("create bt_receivecmd_timer failed\n");

			goto start_exit;
		}
		rt_timer_start(bt_receivecmd_timer);

	    printf("rt_timer_start end\n");
	    return EPDK_OK;
start_exit:
    if(bt_receivecmd_timer!=RT_NULL)
	{
		rt_timer_stop(bt_receivecmd_timer);
		rt_timer_delete(bt_receivecmd_timer);
		bt_receivecmd_timer = RT_NULL;
	}
	if(bt_decodecmd_timer!=RT_NULL)
	{
		rt_timer_stop(bt_decodecmd_timer);
		rt_timer_delete(bt_decodecmd_timer);
		bt_decodecmd_timer = RT_NULL;
	}
	return EPDK_FAIL;
}

__s32 rtt_bt_task_stop(void)
{
    if(bt_receivecmd_timer!=RT_NULL)
	{
		rt_timer_stop(bt_receivecmd_timer);
		rt_timer_delete(bt_receivecmd_timer);
		bt_receivecmd_timer = RT_NULL;
	}
	if(bt_decodecmd_timer!=RT_NULL)
	{
		rt_timer_stop(bt_decodecmd_timer);
		rt_timer_delete(bt_decodecmd_timer);
		bt_decodecmd_timer = RT_NULL;
	}
	return EPDK_OK;
}
#endif
void rtt_msleep(uint16_t ticks)
{
    rt_thread_mdelay(ticks);
    return;
}
#endif
#ifdef __cplusplus
}
#endif
