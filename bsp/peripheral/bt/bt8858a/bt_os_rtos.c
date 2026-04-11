
#include "bt_config.h"
#include "bt_api.h"
#include "bt_os.h"
#include "bt_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#if RTOS


#include "log.h"
#include "kapi.h"
#include "apps.h"
#include "tui.h"
#include "mod_uart.h"
#include "app_root.h"
#include "phonelink_wireless.h"


static __krnl_stmr_t   	*bt_receivecmd_timer;
static __krnl_stmr_t   	*bt_decodecmd_timer;

static uint32_t     	uart_recive_task_id;
static uint32_t   	    uart_decode_task_id;


/**************************bt uart******************************
以下函数是系统层参考函数，需要用户端实现,
uart通讯默认波特率115200，8位数据位，1位停止位，无校验位
**************************bt uart******************************/

//__u32  	COM_UART_BAUD = 115200; 
//bt uart operate

__s32 sys_bt_get_uart_index (u32 *uart_index)//uart index bt
{
	__s32 ret = -1;
	__s32 uart_id = -1;
	
	ret = esCFG_GetKeyValue("bt_para", "bt_uart_id", (__s32 *)&uart_id, 1);
    if(ret < 0)
    {
        __err("get bt_uart_id fail");
        return EPDK_FAIL;
    }
    *uart_index = uart_id;
     return EPDK_OK;
 } 

#if  1//openmode
int32_t g_fp_uart = -1;
 
__s32 rtos_com_uart_init(void)
{
	__s32 ret; 
	__uart_para_t uart_para;
    __s32 uart_index = -1;
	// __wrn("reg uart0 pf(0x%x)",readl( 0x020000f0));
	 	

		//__msg("g_fp_uart open 1");
		sys_bt_get_uart_index(&uart_index);
		if(uart_index == 0)
		{
	        g_fp_uart = open("/dev/uart0",O_RDWR); 
	    }
	    else if(uart_index == 1)
		{
	        g_fp_uart = open("/dev/uart1",O_RDWR); 
	    }
	    else if(uart_index == 2)
		{
	        g_fp_uart = open("/dev/uart2",O_RDWR); 
	    }
	    else if(uart_index == 3)
		{
	        g_fp_uart = open("/dev/uart3",O_RDWR); 
	    }
	    else if(uart_index == 4)
		{
	        g_fp_uart = open("/dev/uart4",O_RDWR); 
	    }
	    else if(uart_index == 5)
		{
	        g_fp_uart = open("/dev/uart5",O_RDWR); 
	    }
	    if(NULL == g_fp_uart)
	    {	     
	        __err(" open bt uart fail!  /dev/uart%d",uart_index);
	    }   
		//__msg("g_fp_uart open 4");
		bt_msleep(10);
		__wrn("UART open success");
	
	return EPDK_OK;
}

__s32 rtos_com_uart_deinit(void)
{

		if(g_fp_uart >= 0)
		{
			__msg("close UART handle");
			close(g_fp_uart);
			g_fp_uart = NULL;
		}
		
		bt_msleep(10);
		__msg("com_uart_deinit end\n");
	
	return EPDK_OK;
}

__s32 rtos_com_uart_write(char* pbuf, __s32 size)
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
		ret = write(g_fp_uart, pbuf, (__size)size);
		//__msg("com_uart_write 3\n");
		if(ret > 0)
		{
			//__msg("com_uart_write 4\n");
			size =- ret;
			pbuf += ret;
			//__msg("has write =%d\n", ret);
		}
	}
	//__msg("com_uart_write end:%s\n",pbuf);
	return EPDK_OK;
}

__s32 rtos_com_uart_read(char* pbuf, __s32 buf_size, __s32* size)
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
	ret = read(g_fp_uart, pbuf, to_read_size); 
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
	//__msg("%d\n",read(g_fp_uart, pbuf, to_read_size));
	//__msg("======read ret%d,==%s======\n",ret,pbuf);
	*size = ret;
	//__msg("---------com_uart_read end\n");
	return EPDK_OK;
}

__s32 rtos_com_uart_flush(void)
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


#else //test ok
ES_FILE	*g_fp_uart = NULL;
__s32 aw_com_uart_init(void)
{
	__s32 ret; 
	__uart_para_t  uart_para;
    __s32 uart_index = -1;

	// __wrn("reg uart0 pf(0x%x)",readl( 0x020000f0));
	 
	
	//__msg("uart_init:%d",uart_init);
	if(0 == uart_init)
	{
		uart_init = 1;
		//__msg("g_fp_uart open 1");
		sys_bt_get_uart_index(&uart_index);
		if(uart_index == 0)
		{
	        g_fp_uart = eLIBs_fopen("b:\\BUS\\UART0", "rb+");
	    }
	    else if(uart_index == 1)
		{
	        g_fp_uart = eLIBs_fopen("b:\\BUS\\UART1", "rb+");
	    }
	    else if(uart_index == 2)
		{
	        g_fp_uart = eLIBs_fopen("b:\\BUS\\UART2", "rb+");
	    }
	    else if(uart_index == 3)
		{
	        g_fp_uart = eLIBs_fopen("b:\\BUS\\UART3", "rb+");
	    }
	    else if(uart_index == 4)
		{
	        g_fp_uart = eLIBs_fopen("b:\\BUS\\UART4", "rb+");
	    }
	    else if(uart_index == 5)
		{
	        g_fp_uart = eLIBs_fopen("b:\\BUS\\UART5", "rb+");
	    }
	    if(NULL == g_fp_uart)
	    {	     
	        __err(" open bt uart fail!  b:\\BUS\\UART[%d]",uart_index);
	    }   
		//__msg("g_fp_uart open 4");
		Msleep(10);
		__wrn("UART open success");
	}
	
	return EPDK_OK;
}

__s32 aw_com_uart_deinit(void)
{
	if(1 == uart_init)
	{
		if(NULL != g_fp_uart)
		{
			//__err("close UART handle");
			eLIBs_fclose(g_fp_uart);
			g_fp_uart = NULL;
		}
		uart_init = 0;
		Msleep(10);
		//__msg("com_uart_deinit end\n");
	}
	return EPDK_OK;
}

__s32 aw_com_uart_write(char* pbuf, __s32 size)
{
	__s32 ret = 0;
	if(NULL == g_fp_uart)
	{
		__err("g_fp_uart handle is NULL\n");
		return EPDK_FAIL;
	}
	//__msg("com_uart_write start:%s,%d",pbuf,size);
	while(size > 0)
	{
		//__msg("com_uart_write %x\n",g_fp_uart);
		ret = eLIBs_fwrite(pbuf, 1, (__size)size, g_fp_uart);
		//__msg("eLIBs_fwrite \n");
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

__s32 aw_com_uart_read(char* pbuf, __s32 buf_size, __s32* size)
{
	__s32 ret = 0;
	__s32 i=0;
	__s32 to_read_size;

	if(NULL == g_fp_uart)
	{
		__err("g_fp_uart is NULL\n");
		return EPDK_FAIL;
	}
	if(NULL == size)
	{
		__err("invalid para size\n");
		return EPDK_FAIL;
	}
	to_read_size = buf_size;
	memset(pbuf, 0, buf_size);
	ret = eLIBs_fread(pbuf, 1, buf_size, g_fp_uart); 
	if(ret>0)
	{
		//__inf("====bt string====\n");
		//for(i=0;i<ret;i++)
		//{
		//	__inf("=%x=",pbuf[i]);
		//}
		//__inf("\n====string voer====\n");
	}
	 //   //__msg("======read ret%d,==%s======\n",ret,pbuf);
	////__msg("%d\n",eLIBs_fread(pbuf, 1, to_read_size, g_fp_uart));
	////__msg("======read ret%d,==%s======\n",ret,pbuf);
	*size = ret;
	////__msg("---------com_uart_read end\n");
	return EPDK_OK;
}

__s32 aw_com_uart_flush(void)
{
	__s32 ret = 0;
		
	if(NULL != g_fp_uart)
	{
	    ret = eLIBs_fioctrl(g_fp_uart, UART_CMD_FLUSH, 0, 0);
	    if(ret != EPDK_OK)
		{
			__err("UART FLUSH fail");
			return EPDK_FAIL;
		}
		Msleep(5);
	}
	return EPDK_OK;
}

////1-init,0-uninit
__s32 aw_com_uart_state(void)
{
	return uart_init;
}
#endif


#if (runmode == 1)//线程运行

void sys_bt_decode_task(void*parg)
{
    while(1)
    {
        if(esKRNL_TDelReq(EXEC_prioself) == OS_TASK_DEL_REQ)
        {
            __wrn("Delete wireless_task!\n");
            goto _exit_bt_decode_task;
        }
        sys_bt_decode_cmd(parg);
        //rtos_msleep(2);

    }
    _exit_bt_decode_task:
    __wrn("---------sys_bt_decode_task exit!----------\n");
    esKRNL_TDel(EXEC_prioself);

}
void sys_bt_receive_task(void*parg)
{
    while(1)
    {
        if(esKRNL_TDelReq(EXEC_prioself) == OS_TASK_DEL_REQ)
        {
            __wrn("Delete wireless_task!\n");
            goto _exit_bt_decode_task;
        }
        sys_bt_receive_cmd(parg);
        //rtos_msleep(2);

    }
    _exit_bt_decode_task:
    __wrn("---------sys_bt_decode_task exit!----------\n");
    esKRNL_TDel(EXEC_prioself);

}

__s32 rtos_bt_task_start(void)//task
{

		printf("uart_key_start\n");
		uart_decode_task_id = esKRNL_TCreate(sys_bt_decode_task, (void*)NULL, 0x800, KRNL_priolevel2);
		if(uart_decode_task_id == NULL)
		{
			printf("create uart_decode_task_id failed\n");
			return EPDK_FAIL;
		}	
		
		uart_recive_task_id = esKRNL_TCreate(sys_bt_receive_task, (void*)NULL, 0x800, KRNL_priolevel1);
		if(uart_recive_task_id == NULL)
		{
			printf("create uart_recive_task_id failed\n");
			return EPDK_FAIL;
		}
		printf("uart_key_start");
	    return EPDK_OK;	
}

__s32 rtos_bt_task_stop(void)
{
	if(uart_recive_task_id != 0)
    {
        do  
        {
            esKRNL_TimeDlyResume(uart_recive_task_id);
            esKRNL_TimeDly(1);
        } while (OS_TASK_NOT_EXIST != esKRNL_TDelReq(uart_recive_task_id));
        uart_recive_task_id = 0;
    }
    if(uart_decode_task_id != 0)
    {
        do  
        {
            esKRNL_TimeDlyResume(uart_decode_task_id);
            esKRNL_TimeDly(1);
        } while (OS_TASK_NOT_EXIST != esKRNL_TDelReq(uart_decode_task_id));
        uart_decode_task_id = 0;
    }
	return EPDK_OK;
}

#elif(runmode == 0)//定时器运行
__s32 rtos_bt_task_start(void) //timer
{

		printf("esTIME_StartTimer bt_decodecmd_timer\n");
		bt_decodecmd_timer = esKRNL_TmrCreate(20,OS_TMR_OPT_PRIO_HIGH | OS_TMR_OPT_PERIODIC,sys_bt_decode_cmd, NULL);
		if(bt_decodecmd_timer == NULL)
		{
			printf("create bt_decodecmd_timer failed\n");

			return EPDK_FAIL;
		}
		esKRNL_TmrStart(bt_decodecmd_timer);
			printf("esTIME_StartTimer bt_receivecmd_timer\n");
		bt_receivecmd_timer = esKRNL_TmrCreate(80,OS_TMR_OPT_PRIO_LOW | OS_TMR_OPT_PERIODIC,sys_bt_receive_cmd, NULL);
		if(bt_receivecmd_timer == NULL)
		{
			printf("create bt_receivecmd_timer failed\n");

			return EPDK_FAIL;
		}
		esKRNL_TmrStart(bt_receivecmd_timer);
	
	    printf("esKRNL_TmrStart end\n");
	    return EPDK_OK;
start_exit:
    if(bt_receivecmd_timer!=RT_NULL)
	{
		esKRNL_TmrStop(bt_receivecmd_timer,OS_TMR_OPT_PRIO_LOW | OS_TMR_OPT_PERIODIC,NULL);
		esKRNL_TmrDel(bt_receivecmd_timer);
		bt_receivecmd_timer = RT_NULL;
	}
	if(bt_decodecmd_timer!=RT_NULL)
	{
		esKRNL_TmrStop(bt_decodecmd_timer,OS_TMR_OPT_PRIO_LOW | OS_TMR_OPT_PERIODIC,NULL);
		esKRNL_TmrDel(bt_decodecmd_timer);
		bt_decodecmd_timer = RT_NULL;
	}
	return EPDK_FAIL;
}

__s32 rtos_bt_task_stop(void)
{
	if(bt_receivecmd_timer!=RT_NULL)
	{
		esKRNL_TmrStop(bt_receivecmd_timer,OS_TMR_OPT_PRIO_LOW | OS_TMR_OPT_PERIODIC,NULL);
		esKRNL_TmrDel(bt_receivecmd_timer);
		bt_receivecmd_timer = RT_NULL;
	}
	if(bt_decodecmd_timer!=RT_NULL)
	{
		esKRNL_TmrStop(bt_decodecmd_timer,OS_TMR_OPT_PRIO_LOW | OS_TMR_OPT_PERIODIC,NULL);
		esKRNL_TmrDel(bt_decodecmd_timer);
		bt_decodecmd_timer = RT_NULL;
	}
	return EPDK_OK;
}

#endif
void rtos_msleep(uint16_t ticks)
{
    esKRNL_TimeDly(ticks);
    return;
}

#endif

#ifdef __cplusplus
}
#endif

