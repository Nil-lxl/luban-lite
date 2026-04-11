#include "atbm_hal.h"
#if 1
struct atbm_test_item{
	char *name;
	int (*test_func)(void);
};
extern struct atbmwifi_common g_hw_prv;
extern int atbm_direct_read_reg_32(struct atbmwifi_common *hw_priv, atbm_uint32 addr, atbm_uint32 *val);
struct packed_test{
	char data1;
	int data2;
	short data3;
}atbm_packed;

int atbm_packed_test(){
	if(sizeof(struct packed_test) != 7)
		return -1;
	return 0;
}

int atbm_GetOsTimeTest(){
	if((atbm_GetOsTime() <= 0) ||(atbm_GetOsTimeMs() <= 0)){
		return -1;
	}
	return 0;
}

int atbm_SleepMsTest(){
	atbm_uint32 start, end;
	start = atbm_GetOsTime();
	atbm_SleepMs(1000);
	end = atbm_GetOsTime();
	//printk("end - start=%u\n",end - start);
	if((end - start) > 1100 || (end - start < 900)){
		return -1;
	}
	start = atbm_GetOsTime();
	atbm_mdelay(1000);
	end = atbm_GetOsTime();
	//printk("end - start=%u\n",end - start);
	if((end - start) > 1100 || (end - start < 900)){
		return -2;
	}
	return 0;
}

int atbm_thread_start = 0;
static int atbm_thread_arg_test_fail = 0;
void atbm_test_thread(atbm_void *p_arg){
	if(((int)p_arg) != 0x5555){
		atbm_thread_arg_test_fail = 1;
	}
	atbm_thread_start = 1;
	while(1){
		atbm_SleepMs(100);
	}
}

int atbm_thread_test(){
	int retry = 3;
	int arg = 0x5555;
	pAtbm_thread_t thread;
	//wifi_printk(WIFI_ALWAYS, "retry:%p arg:%p thread:%p atbm_thread_start:%p atbm_thread_arg_test_fail:%p\n", &retry, &arg, &thread, &atbm_thread_start, &atbm_thread_arg_test_fail);
	thread = atbm_createThread("tt",atbm_test_thread, (void*)arg, TEST_TASK_PRIO);
	if(!thread){
		return -1;
	}
	while((--retry) >= 0){
		if(atbm_thread_arg_test_fail){
			return -2;
		}
		if(atbm_thread_start){
			break;
		}
		atbm_SleepMs(10);
	}
	if(retry < 0){
		return -3;
	}
	if(atbm_stopThread(thread)){
		return -4;
	}
	atbm_thread_arg_test_fail = 0;
	atbm_thread_start = 0;
	return 0;
}

atbm_mutex test_mutex;
int atbm_mutextest_thread_start = 0;
void atbm_mutextest_thread(atbm_void *p_arg){
	atbm_os_mutexLock(&test_mutex, 0xffffffff);
	atbm_mutextest_thread_start = 1;
	atbm_SleepMs(1000);
	atbm_mutextest_thread_start = 2;
	atbm_os_mutexUnLock(&test_mutex);
	atbm_SleepMs(1000);
	atbm_os_mutexLock(&test_mutex, 0xffffffff);
	atbm_mutextest_thread_start = 3;
	while(1){
		atbm_SleepMs(1000);
	}
}

int atbm_mutex_test(){
	pAtbm_thread_t thread;

	if(atbm_os_mutexLockInit(&test_mutex))
		return -1;
	thread = atbm_createThread("test_mutex",atbm_mutextest_thread, NULL, TEST_TASK_PRIO);
	while(!atbm_mutextest_thread_start){
		atbm_SleepMs(10);
	}
	if(atbm_mutextest_thread_start != 1){
		return -2;
	}
	if(atbm_os_mutexLock(&test_mutex, 10*HZ)){
		return -3;
	}
	atbm_os_mutexUnLock(&test_mutex);
	while(atbm_mutextest_thread_start != 3){
		atbm_SleepMs(10);
	}
	if(atbm_os_DeleteMutex(&test_mutex)){
		return -5;
	}
	atbm_stopThread(thread);
	atbm_mutextest_thread_start = 0;
	return 0;
}

atbm_os_wait_queue_head_t test_event;
void atbm_waitEventtest_thread(atbm_void *p_arg){
	atbm_os_wakeup_event(&test_event);
	while(1){
		atbm_SleepMs(1000);
	}
}

int atbm_waitEvent_test(){
	int status;
	pAtbm_thread_t thread;
	atbm_uint32 start, end;

	atbm_os_init_waitevent(&test_event);
	thread = atbm_createThread("test_evt",atbm_waitEventtest_thread, NULL, TEST_TASK_PRIO);
	status = atbm_os_wait_event_timeout(&test_event, 10*HZ);
	if(status != 1){
		return -1;
	}
	start = atbm_GetOsTime();
	status = atbm_os_wait_event_timeout(&test_event, HZ);
	if(status != 0){
		return -2;
	}
	end = atbm_GetOsTime();
	//printk("end - start=%u\n",end - start);
	if((end - start < 900) || (end - start > 1100)){
		return -3;
	}
	atbm_stopThread(thread);
	return 0;
}

void atbm_timeout_func(void * userdata1){
	atbm_os_wait_queue_head_t *event = (atbm_os_wait_queue_head_t *)userdata1;
	atbm_os_wakeup_event(event);
}

int atbm_timer_test(){
	OS_TIMER Timer;
	atbm_os_wait_queue_head_t event;
	atbm_uint32 start, end;

	atbm_os_init_waitevent(&event);
	if(atbm_InitTimer(&Timer, atbm_timeout_func, &event))
		return -1;
	start = atbm_GetOsTime();
	if(atbm_StartTimer(&Timer, 1000))
		return -2;
	atbm_os_wait_event_timeout(&event, 10*HZ);
	end = atbm_GetOsTime();
	//printk("end - start=%u\n",end - start);
	if((end - start < 900) || (end - start > 1100)){
		return -3;
	}
	return 0;
}

int msgPool[4];

void atbm_MsgQtest_thread(atbm_void *p_arg){
    int flag = 0x5555;
    atbm_os_msgq *msgQ = (atbm_os_msgq *)p_arg;
    atbm_os_MsgQ_Send(msgQ, &flag, 4, 0xffffffff);
    while(1){
        atbm_SleepMs(1000);
    }
}

int atbm_MsgQ_test(){
    atbm_os_msgq msgQ;
    pAtbm_thread_t thread;
    int flag;
    if(atbm_os_MsgQ_Create(&msgQ, msgPool, sizeof(int), 4))
        return -1;
    thread = atbm_createThread("test_msg",atbm_MsgQtest_thread, &msgQ, TEST_TASK_PRIO);
    if(atbm_os_MsgQ_Recv(&msgQ, &flag, sizeof(int), 0xffffffff))
        return -2;
    if(flag != 0x5555)
        return -3;
    atbm_stopThread(thread);
    atbm_os_MsgQ_Delete(&msgQ);
    return 0;
}
int atbm_timesync_test(){
	unsigned int pre, now;
	struct atbmwifi_common	*hw_priv = &g_hw_prv;
	if(atbm_direct_read_reg_32(hw_priv, 0x1640006c, &pre) < 0){
		return -1;
	}
	atbm_SleepMs(1000);
	if(atbm_direct_read_reg_32(hw_priv, 0x1640006c, &now) < 0){
		return -2;
	}
	if((now - pre < 950000) || (now - pre > 1050000)){
		return -3;
	}
	return 0;
}


int atbm_txrx_test(){
	int time = 10*HZ;
	struct atbmwifi_common	*hw_priv = &g_hw_prv;
	unsigned int band_width = 0;
	unsigned int start = atbm_GetOsTime();
	unsigned int now = start;

	while((atbm_GetOsTime() - start) < time){
		if(wsm_txrx_data_test(hw_priv, 1500, 0))
			return -1;
		band_width += 1500;
		if((atbm_GetOsTime() - now) >= HZ){
			band_width = band_width << 3;
			wifi_printk(WIFI_ALWAYS, "band width %uM%uK\n", band_width>>20, (band_width>>10)&0x3ff);
			now = atbm_GetOsTime();
			band_width = 0;
		}
	}
	return 0;
}

struct atbm_test_item items[] = {
	{"atbm_packed", atbm_packed_test},
	{"atbm_GetOsTime", atbm_GetOsTimeTest},
	{"atbm_SleepMs", atbm_SleepMsTest},
	{"atbm_thread", atbm_thread_test},
	{"atbm_mutex", atbm_mutex_test},
	{"atbm_waitEvent", atbm_waitEvent_test},
	{"atbm_timer", atbm_timer_test},
	{"atbm_MsgQ", atbm_MsgQ_test},
	/*following test items can only be executed after the setup of SDIO/USB interface*/
	{"atbm_timesync", atbm_timesync_test},
	{"atbm_txrx", atbm_txrx_test},
	{NULL, NULL},
};

int atbm_func_test(){
	int i, ret;
	for(i = 0; items[i].test_func != NULL; i++){
		ret = items[i].test_func();
		if(ret){
			wifi_printk(WIFI_ALWAYS, "%d.Test item[%s] failed[%d]!!\n", i+1, items[i].name, ret);
		}else{
			wifi_printk(WIFI_ALWAYS, "%d.Test item[%s] passed!!\n", i+1, items[i].name);
		}
	}
	return 0;
}

int atbm_func_test_item(int item){
	int ret;

	ret = items[item-1].test_func();
	if(ret){
		wifi_printk(WIFI_ALWAYS, "%d.Test item[%s] failed[%d]!!\n", item, items[item-1].name, ret);
	}else{
		wifi_printk(WIFI_ALWAYS, "%d.Test item[%s] passed!!\n", item, items[item-1].name);
	}
	return 0;
}
#ifdef RT_USING_FINSH
#include "finsh.h"
MSH_CMD_EXPORT(atbm_func_test, atbm_func_test);
#endif
#endif
