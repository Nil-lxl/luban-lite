
#include "bt_config.h"
#include "bt_api.h"
#include "bt_os.h"
#include "bt_core.h"

#ifdef __cplusplus
extern "C" {
#endif


#if LINUX

static int32_t   	bt_receivecmd_timer;
static int32_t   	bt_decodecmd_timer;

static pthread_t   	uart_recive_task_id;
static pthread_t   	uart_decode_task_id;


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

__s32 bt_task_start(void)//task
{
        int     err;
        
		printf("bt_task_start\n");
		err = pthread_create(&uart_decode_task_id, NULL, sys_bt_decode_task, (void*)NULL);
        if (err) 
        {
            printf("decode_task pthread_create error!");
            return EPDK_FAIL;
        }
        printf("create uart_decode_task_id:0x%x", uart_decode_task_id);
       
		err = pthread_create(&uart_recive_task_id, NULL, sys_bt_receive_task, (void*)NULL);
        if (err) 
        {
            printf("recive_task pthread_create error!");
            return EPDK_FAIL;
        }
        printf("create uart_recive_task_id:0x%x", uart_recive_task_id);
        
	    return EPDK_OK;	
}

__s32 bt_task_stop(void)
{
	if(uart_recive_task_id != 0)
    {
        pthread_cancel(uart_recive_task_id);
        uart_recive_task_id = 0;
    }
    if(uart_decode_task_id != 0)
    {
        pthread_cancel(uart_decode_task_id);
        uart_decode_task_id = 0;
    }
	return EPDK_OK;
}

#elif(runmode == 0)//定时器运行
__s32 bt_task_start(void) //timer
{
        struct itimerspec its1;    
        timer_t timerid1;     // 初始化 itimerspec 结构体    
        struct sigevent se1;
        struct itimerspec its2;    
        timer_t timerid2;     // 初始化 itimerspec 结构体    
        struct sigevent se2;
         // 设置定时器事件    
        se1.sigev_notify = SIGEV_THREAD;  // 使用线程处理方式    
        se1.sigev_notify_function = sys_bt_decode_cmd;  // 设置线程处理函数    
        se1.sigev_notify_attributes = NULL;

		printf("timer_create bt_decodecmd_timer\n");
		timer_create(CLOCK_REALTIME, &se1, &timerid1)
		if(timerid1 == NULL)
		{
			printf("create bt_decodecmd_timer failed\n");
			return EPDK_FAIL;
		}
		        
        its1.it_value.tv_sec = 0;   //设置定时器的初始超时时间为10ms   
        its1.it_value.tv_nsec = 10*1000;    
        its1.it_interval.tv_sec = 0;  // 设置定时器的间隔时间为20ms  
        its1.it_interval.tv_nsec = 20*1000;  
        timer_settime(timerid1, 0, &its1, NULL);
        
        // 设置定时器事件    
        se2.sigev_notify = SIGEV_THREAD;  // 使用线程处理方式    
        se2.sigev_notify_function = sys_bt_receive_cmd;  // 设置线程处理函数    
        se2.sigev_notify_attributes = NULL;
         
		printf("timer_create bt_receivecmd_timer\n");
		timer_create(CLOCK_REALTIME, &se2, &timerid2);	
		if(bt_receivecmd_timer == NULL)
		{
			printf("create bt_receivecmd_timer failed\n");
			return EPDK_FAIL;
		}
		        
        its2.it_value.tv_sec = 0;   //设置定时器的初始超时时间为70ms   
        its2.it_value.tv_nsec = 70*1000;    
        its2.it_interval.tv_sec = 0;  // 设置定时器的间隔时间为80ms  
        its2.it_interval.tv_nsec = 80*1000;  
		timer_settime(timerid2, 0, &its2, NULL);
	
	    printf("timer_settime end\n");
	    
	    return EPDK_OK;
start_exit:
    {
        struct itimerspec its = {{0, 0}, {0, 0}};     
    	if(bt_receivecmd_timer!=0)
    	{
            timer_settime(bt_receivecmd_timer, 0, &its, NULL);
            timer_delete(bt_receivecmd_timer);//对于存放定时器相关数据的结构体可以清空
            bt_receivecmd_timer = 0; 
    	}
    	if(bt_decodecmd_timer!=0)
    	{
    	    timer_settime(bt_decodecmd_timer, 0, &its, NULL);
            timer_delete(bt_decodecmd_timer);//对于存放定时器相关数据的结构体可以清空
            bt_decodecmd_timer = 0; 
    	}
	}
	return EPDK_FAIL;
}

__s32 bt_task_stop(void)
{
    struct itimerspec its = {{0, 0}, {0, 0}};     
	if(bt_receivecmd_timer!=0)
	{
        timer_settime(bt_receivecmd_timer, 0, &its, NULL);
        timer_delete(bt_receivecmd_timer);//对于存放定时器相关数据的结构体可以清空
        bt_receivecmd_timer = 0; 
	}
	if(bt_decodecmd_timer!=0)
	{
	    timer_settime(bt_decodecmd_timer, 0, &its, NULL);
        timer_delete(bt_decodecmd_timer);//对于存放定时器相关数据的结构体可以清空
        bt_decodecmd_timer = 0; 
	}
	return EPDK_OK;
}
#endif
#endif
#ifdef __cplusplus
}
#endif

