
#include "bt_config.h"
#include "bt_api.h"
#include "bt_os.h"
#include "bt_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/////消息队列/////
typedef enum
{
	FIFO_NULL = 0,
	FIFO_HAVE,
	FIFO_FULL,
	FIFO_MAX
}fifo_state_t;



__u8	bt_keycmd_buf[BT_MSG_MAX] 	= {'\0'};
__u8  	system_do_msg[BT_MSG_MAX];				//////系统要处理的消息
__u8  	system_tmp_msg[BT_MSG_MAX];				//////系统要处理的消息
__u8	sys_cmd[BT_CMD_MAX][BT_MSG_MAX]={'\0'};

__s32   fifo_in = 0;
__s32   fifo_out = 0;
__s32 	fifo_sta = FIFO_NULL;



/***********************************************/
/*入队*/
__s32 fifo_push(__u8 *string_in,__u8 len)
{
	__s32 i=0;
	if(fifo_sta == FIFO_FULL)
	{
		//printf("===============err队满:in:%d,out:%d===============\n",fifo_in,fifo_out);
		return 0;
	}
	//printf("-fifoIN [%d]len:%d-",fifo_in,len);
	///入队
	for(i=0;i<=len;i++)
	{
		sys_cmd[fifo_in][i]=string_in[i];
		//printf("-%x-",sys_cmd[fifo_in][i]);
	}
	//printf("fifoin over\n");
	///前移
	if((++fifo_in)>=BT_CMD_MAX)
	{
		fifo_in = 0;
	}
	//队列是否满
	if(fifo_sta == FIFO_NULL)
	{
		//printf("===============null in:%d,out:%d===============\n",fifo_in,fifo_out);
		fifo_sta = FIFO_HAVE;
	}
	else if(fifo_sta == FIFO_HAVE)
	{
		//printf("===============have in:%d,out:%d===============\n",fifo_in,fifo_out);
		if(fifo_in == fifo_out)
		{
			printf("bt msg queue is full\n");
			fifo_sta = FIFO_FULL;
		}
	}
	return 0;
}
/*出队*/
__s32 fifo_pop(__u8 *string_out)
{
	__s32 i=0;
	if(fifo_sta == FIFO_NULL)
	{
		//printf("bt msg queue is empty\n");
		return -1;
	}
	else if(fifo_sta == FIFO_FULL)
	{
		printf("bt msg queue is full\n");
		fifo_sta = FIFO_HAVE;
	}
	//printf("===queue pop====\n");
	///出队
	for(i=0;i<200;i++)///最多200个
	{
		string_out[i] = sys_cmd[fifo_out][i];
		//__msg("-%x-",string_out[i]);
	}
	//printf("=outover=\n");
	///前移
	if((++fifo_out)>=BT_CMD_MAX)
	{
		fifo_out = 0;
	}
	//队列是否为空
	if(fifo_in == fifo_out)
	{
		/////printf("===============队列空===============\n");
		fifo_sta = FIFO_NULL;
	}
	return 0;
}

__s32 bt_cmp_cmd(char *string_buf,__s32 str_len)
{
	__s32 i;//,cmdindex=0;
	__s32 ret = -1;
	char    *p=string_buf;
	char    *p_end = string_buf;
	static	__s32 	index = 0;

	p_end+= str_len;
	while(p<p_end)///取出命令
	{
		//printf("get len:[%d][%x]\n",str_len,*p);
		///开始
#if(comm_mode == 1)
		if(strncmp(p,"\r\n",2)==0)////有信息了"\r\nk012345678\r\n"取出的为:"012345678"
        {
			p = p+1;
#elif(comm_mode == 2)
        if(strncmp(p,"\\r\\n",4)==0)////有信息了"\\r\\nk012345678\\r\\n"取出的为:"012345678"
        {
			p = p+3;
#elif (comm_mode == 3)
		if(*p == 0xFF)////有信息了"\r\nk012345678\r\n"取出的为:"012345678"
		{
#endif
            // printf("system_tmp_msg[0] =%x",system_tmp_msg[0]);
            if(index>0)
			{
				//printf("1 push msg:[%d][%x]\n",index,system_tmp_msg[index]);
				fifo_push(system_tmp_msg,index);
				index=0;
				memset(system_tmp_msg, 0, BT_MSG_MAX);
			}
		}
		else
		{
			system_tmp_msg[index]=*p;
			//printf("2 push msg:[%d][%x]\n",index,system_tmp_msg[index]);
			++index;
		}
		++p;
	}
	return 0;
}

 void sys_bt_receive_cmd(void *parg)
{
	__s32 		ret 			= -1;
	__s32 		i 			= 0;
	static 		__s32 		clear_cnt=0;
	com_uart_read(bt_keycmd_buf, BT_MSG_MAX-1, &ret);
	if(ret>0)
	{
		bt_cmp_cmd(bt_keycmd_buf,ret);
		++clear_cnt;
	}
	else
	{
		if(clear_cnt>100)
		{
			clear_cnt=0;
			printf("=====cear uart buffer====\n");
			com_uart_flush();
		}
        msleep(10);
	}
	return;
}

 void sys_bt_decode_cmd(void*parg)
{
	__s32 ret = -1;
	__s32  i;
	ret = fifo_pop(system_do_msg);
	if(ret == -1)
	{
		msleep(10);
		return ;
	}
	//printf("system_do_msg[0]:0x%x 0x%x 0x%x \n",system_do_msg[0],system_do_msg[1],system_do_msg[2]);
	switch(system_do_msg[0])
	{
		case 'B':////电话本
		{
			printf("system_do_msg[0]:%x \n",system_do_msg[0]);
			sys_bt_set_phonebook(system_do_msg);
		}
		break;
		case 'C'://///控制命令
		{
			switch(system_do_msg[1])
			{
				case 'A':
				{
				    sys_bt_ctrl_lcd(system_do_msg[2]);
				}
				break;
				case 'I':///大灯
				{
                    sys_bt_ctrl_light(system_do_msg[2]);
				}
				break;
				default:
				break;
			}
		}
		break;
		case 'D':///方向键
		{
			printf("**************Direction key***************\n");
			//sys_bt_dirkey_pro(system_do_msg);
		}
		break;
		case 'E':///FM返回的信息
		{
			printf("**************fm_process***************\n");
			sys_bt_fm_process();
		}
		break;
		case 'I':///bt
		{
			printf("*************bt :%c-%c***************\n",system_do_msg[0],system_do_msg[1]);
			sys_bt_dosomething();
		}
		break;

		case 'K':///key///
		{
			printf("===bt=key ====\n");
			//hyq modify
			//sys_bt_key_pro(system_do_msg);
		}
		break;
		case 'P':////倒车
		{

			sys_bt_ctrl_pev(system_do_msg[1]);
		}
		break;
		case 'R':////ir 按键
		{
			///printf("====ir:[%x]sta:[%d][0:up 1:down]====\n",system_do_msg[1],system_do_msg[2]);
			//cky modify
			//sys_bt_irkey_pro(system_do_msg);
		}
		break;
		case 'T':///T LEN YEAR....
		{
			sys_bt_set_time(&system_do_msg[2]);//system_do_msg[1]为长度

		}
		break;
		case 'V':///vol
		{
			printf("==========vol========\n");
			//sys_bt_volkey_pro(system_do_msg);
		}
		break;
		case 'S':///vol
		{
			//printf("==========id3========\n");
			sys_bt_set_music_info();
		}
		break;
		case 'M':///音乐播放状态
		{
			sys_bt_set_music_status(system_do_msg[1]);
		}
		break;
		case '#':///SPP协议部分
		{
			printf("=====SPP strlen[%d],cmd:%c%c\n\n",strlen(system_do_msg),system_do_msg[1],system_do_msg[2]);
			if(system_do_msg[1]=='P')
			{
                bt_spp_rx_data(&system_do_msg[1],12);
			}
			else
			{
			    bt_spp_rx_data(&system_do_msg[1],strlen(system_do_msg)-1);
			}
		}
		break;
		case '*':///ble协议部分
		{
			printf("=====BLE strlen[%d],cmd:%c%c\n\n",strlen(system_do_msg),system_do_msg[1],system_do_msg[2]);
		//	bt_ble_rx_data(&system_do_msg[1],strlen(system_do_msg)-1);

		}
		break;
		case '@':///hid协议部分
		{
			printf("=====HID strlen[%d],cmd:%c%c\n\n",strlen(system_do_msg),system_do_msg[1],system_do_msg[2]);
			bt_hid_rx_data(&system_do_msg[1],strlen(system_do_msg)-1);
		}
		break;
		default:
		{

		}
	    break;
	}
	return ;
}


__s32 bt_init(void)
{
    bt_para_init();
	com_uart_init();
	com_uart_flush();
	bt_task_start();
    return EPDK_OK;

}

__s32 bt_exit(void)
{
    bt_task_stop();
	com_uart_flush();
    com_uart_deinit();
    bt_para_exit();
    return EPDK_OK;
}

__s32 check_pev_key(void)
{
	__s32 		ret 		= EPDK_FAIL;
	__s32 		cnt			= 0;
	__s32 		i=0;
	//for(i=0;i<3;i++)
	{
		com_uart_read(bt_keycmd_buf, BT_MSG_MAX-1, &cnt);
		if(cnt>0)
		{
			__s32 i=0;

		 /*
			for(i=0;i<cnt;i++)
			{
				printf("-%x-",bt_keycmd_buf[i]);
			}
	    */
			//////__msg("===read ok:%s===\n",key_buf);
			bt_cmp_cmd(bt_keycmd_buf,cnt);

			while(fifo_pop(system_do_msg)!=-1)
			{
				switch(system_do_msg[0])
				{
					case 'P':////倒车
					{
						///printf("=====pev:%x,%x====\n",system_do_msg[1],system_do_msg[2]);
						if(system_do_msg[1]==1)///pev  1->0  2->1
						{
							printf("=================prv enter==============\n");
            				return EPDK_OK;
						}
						else if(system_do_msg[1]==0)///exit pev
						{
							printf("=================prv exit==============\n");
							ret = EPDK_FAIL;
						}
					}
					break;
					case 'T':///T LEN YEAR....
					{
            			sys_bt_set_time(&system_do_msg[2]);//system_do_msg[1]为长度
            		}
					break;
					default:
					break;
				}
			}
		}

	}
	return ret;
}


#ifdef __cplusplus
}
#endif
