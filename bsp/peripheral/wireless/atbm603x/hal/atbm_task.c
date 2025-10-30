/**************************************************************************************************************
 * altobeam RTOS
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#include "atbm_hal.h"
#include "atbm_task.h"
#if QUEUE_LIST
static atbm_void atbm_workqueue_thread(atbm_void *arg)
{
	atbm_uint32 ret;
	atbm_uint32 irq_flag;
	struct atbmwifi_common *hw_priv = (struct atbmwifi_common *)arg;
	while (1)
	{
		atbm_os_wait_event_timeout(&hw_priv->work_wq,10*HZ);
		
		if(hw_priv->bh_term){
			break;
		}
		irq_flag = atbm_local_irq_save();
		while (!atbm_list_empty(&hw_priv->work_Link))
		{
			struct atbmwifi_work_struct *work = NULL;
			irq_flag = atbm_local_irq_save();
			work=atbm_list_first_entry(&hw_priv->work_Link,struct atbmwifi_work_struct,workLink);
			atbm_local_irq_restore(irq_flag);
			if(work->valid){
				if(work->fun){
					work->fun(work->data);
				}
				irq_flag = atbm_local_irq_save();
				atbm_list_del(&work->workLink);
			}else{
				break;
			}
			atbm_local_irq_restore(irq_flag);
		}
	}
}
atbm_work  atbm_init_work(struct atbmwifi_common *hw_priv,atbm_void *fun, atbm_void *data)
{
	struct atbmwifi_work_struct *work = NULL;
	work = (struct atbmwifi_work_struct *)atbm_kmalloc(sizeof(struct atbmwifi_work_struct),GFP_KERNEL);	
	if(work){
		work->fun = (atbm_void (*)(atbm_void *))fun;
		work->data = data;
		work->valid=1;
	}
	return work;
}
atbm_void  atbm_cancel_work(struct atbmwifi_common *hw_priv,struct atbmwifi_work_struct *work)
{
	if(work->valid){
		work->valid=0;
		atbm_kfree(work);
	}
	work=ATBM_NULL;
}

atbm_uint32 atbm_queue_work(struct atbmwifi_common *hw_priv, struct atbmwifi_work_struct *work)
{
	atbm_uint32 irq_flg;
	irq_flg = atbm_local_irq_save();
	atbm_list_add_tail(&work->workLink, &hw_priv->work_Link);
	atbm_local_irq_restore(irq_flg);
	atbm_os_wakeup_event(&hw_priv->work_wq);
	return 0;
}
int atbm_create_workqueue(struct atbmwifi_common *hw_priv)
{
	atbm_uint32 ret;
	atbm_os_init_waitevent(&hw_priv->work_wq);
	ATBM_INIT_LIST_HEAD(&hw_priv->work_Link);
	hw_priv->work_queue_thread=atbm_createThreadInternal(atbm_workqueue_thread,(atbm_void*)hw_priv,WORK_TASK_PRIO);
	if (!hw_priv->work_queue_thread){
		wifi_printk(WIFI_IF,"work_queue_thread Failed\n");
		ret = WIFI_ERROR;
	}else{
		ret = WIFI_OK;
	}

	return ret;
}

int atbm_destory_task_work(struct atbmwifi_common *hw_priv)
{
	int ret = 0;
	//wifi_printk(WIFI_ALWAYS,"atbm_destory_task_work\n");
	hw_priv->bh_term=1;
	atbm_os_wakeup_event(&hw_priv->work_wq);
	atbm_stopThread(hw_priv->work_queue_thread);
	atbm_os_delete_waitevent(&hw_priv->work_wq);
	return ret;
}
#else

int atbm_task_work(atbm_void *arg);
int atbm_create_workqueue(struct atbmwifi_common *hw_priv)
{
	int ret;
	atbm_memset(&hw_priv->work_queue_table[0],0,sizeof(struct atbmwifi_work_struct)*ATBM_WIFI_MAX_WORKQUEUE);
	atbm_os_init_waitevent(&hw_priv->work_wq);
	hw_priv->work_map =0;
	//sema_init(&hw_priv->work_wq,0);
	hw_priv->work_queue_thread=atbm_createThreadInternal("atbm_work",atbm_task_work,(atbm_void*)hw_priv,WORK_TASK_PRIO);
	if (!hw_priv->work_queue_thread){
		wifi_printk(WIFI_IF,"work_queue_thread Failed\n");
		ret = WIFI_ERROR;
	}else{
		ret = WIFI_OK;
	}

	return ret;
}
int atbm_destory_task_work(struct atbmwifi_common *hw_priv)
{
	int ret = 0;
	//wifi_printk(WIFI_ALWAYS,"atbm_destory_task_work\n");
	hw_priv->bh_term=1;
	atbm_os_wakeup_event(&hw_priv->work_wq);
	atbm_stopThread(hw_priv->work_queue_thread);
	atbm_os_delete_waitevent(&hw_priv->work_wq);
	return ret;
}

int atbm_task_work(atbm_void *arg)
{

	struct atbmwifi_common *hw_priv = (struct atbmwifi_common *)arg;
	int id=0;
	struct atbmwifi_work_struct *work;

	while(1){		
	
		atbm_os_wait_event_timeout(&hw_priv->work_wq,10*HZ);
		if(hw_priv->bh_term)
			break;
		for(id=0;id<ATBM_WIFI_MAX_WORKQUEUE;id++){		
			work = &hw_priv->work_queue_table[id];
			if(atbm_test_bit(id,&hw_priv->work_map)){		
				atbm_clear_bit(id,&hw_priv->work_map);
				wifi_printk(WIFI_TASK,"work->fun-id %d-\n",id);
				work->fun(work->data);				
			}
			if(hw_priv->work_map ==0)
				break;
		}

	}
	atbm_ThreadStopEvent(hw_priv->work_queue_thread);
	return 0;
}
atbm_void atbm_queue_work(struct atbmwifi_common *hw_priv,atbm_work workid)
{
	struct atbmwifi_work_struct *work;

	
	wifi_printk(WIFI_IF,"atbm_queue_work1++ workid %d\n",workid); 
	work = &hw_priv->work_queue_table[workid];
	if(work->valid ==0){
		wifi_printk(WIFI_ALWAYS,"atbm_queue_work error !\n");
		return;
	}
	if(atbm_atomic_read(&work->pending)==0){
		wifi_printk(WIFI_TASK,"atbm_queue_work _wakeup_\n");
		//atbm_atomic_set(&work->pending,1);
		atbm_set_bit(workid,&hw_priv->work_map);
		atbm_os_wakeup_event(&hw_priv->work_wq);
	}
}
atbm_work atbm_init_work(struct atbmwifi_common *hw_priv,atbm_void *fun, atbm_void *data)
{
	int id=0;
	struct atbmwifi_work_struct *work;
	for(id=0;id<ATBM_WIFI_MAX_WORKQUEUE;id++){		
		work = &hw_priv->work_queue_table[id];
		if(work->valid ==0){
			work->fun = (atbm_void (*)(atbm_void *))fun;
			work->data = data;
			work->index= id;
			work->valid= 1;	
			break;
		}
	}
	return id;
}

atbm_void  atbm_cancel_work(struct atbmwifi_common *hw_priv,atbm_work work_id)
{
	struct atbmwifi_work_struct *work;

	work = &hw_priv->work_queue_table[work_id];
	if(work->valid == 1){
		work->valid=0;
		work->fun = ATBM_NULL;
		work->data = ATBM_NULL;
		work->index= 0;
	}

	return;
}

#endif

atbm_void  atbm_bh_schedule_tx(struct atbmwifi_common	*hw_priv)
{
#if ATBM_USB_BUS
	 atbm_usb_xmit_data(hw_priv->sbus_priv);
#else
	if (atbm_atomic_add_return(1, &hw_priv->bh_tx) == 1){
#if ATBM_TXRX_IN_ONE_THREAD
		atbm_os_wakeup_event(&hw_priv->bh_wq);
#else
		atbm_os_wakeup_event(&hw_priv->tx_bh_wq);
#endif
	}else {
		wifi_printk(WIFI_IF,"atbm_bh_wakeup not needed\n");
	}
#endif
}
atbm_void atbm_bh_wakeup(struct atbmwifi_common *hw_priv)
{
	if ((hw_priv->bh_error)||atbm_bh_is_term(hw_priv)){
		wifi_printk(WIFI_IF,"atbm_wifi [BH] err drop\n");
		return;
	}
	/*wakeup TxThread to transmit.........*/
	
	atbm_bh_schedule_tx(hw_priv);
}

