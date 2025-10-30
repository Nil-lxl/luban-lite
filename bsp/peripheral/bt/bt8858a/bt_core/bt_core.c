

#include "bt_os.h"

#ifdef __cplusplus
extern "C" {
#endif



#define 		BT_TEXT_LEN					718

static uint8_t bt_play_status = 0;

static u8 tmp_spp_txdata[528]={'\0'};//spp rfcomm 通讯后发送给车机的数据，包含蓝牙命令，第三方库透传过去的数据等
static u8 spp_txdata[528]={'\0'};    //spp rfcomm 通讯后发送给车机的数据，包含蓝牙命令，第三方库透传过去的数据等
u8 spp_rxdata[512]={'\0'}; //rfcomm 通讯后从手机接收透传的数据



//将小写字母转换成大小字母
__s32  lowercase_2_uppercase(char *str)
{
    char *p_str = NULL ;

    if (!str)
    {
        __msg("string is null , return \n");
        return EPDK_FAIL ;
    }

    p_str = str ;

    while (*p_str != '\0')
    {
        if ((*p_str <= 'z') && (*p_str >= 'a'))
        {
            *p_str =  *p_str - 'a' + 'A' ;
        }

        p_str++ ;
    }

    return EPDK_OK ;
}

void BtCmdSend (char *CmdStr,unsigned char len)
{
	char senddata[128]={'\0'};
	char *p = NULL;
	__s32 i = 0;

	p = CmdStr;

#if(comm_mode == 1)
    senddata[0] = 'A';
	senddata[1] = 'T';
    senddata[2] = '#';
    memcpy(&senddata[3],CmdStr,len);

	senddata[len+3]='\r';
	senddata[len+4]='\n';
    __wrn("======send cmd:%s,len:%d======\n",senddata,strlen(senddata));
	com_uart_write(senddata, (5+len));
#elif(comm_mode == 2)
    senddata[0] = 'A';
	senddata[1] = 'T';
    senddata[2] = '#';
    memcpy(&senddata[3],CmdStr,len);

	senddata[len+3]='\\';
	senddata[len+4]='r';
	senddata[len+5]='\\';
	senddata[len+6]='n';
	__msg("======send cmd:%s,len:%d======\n",senddata,eLIBs_strlen(senddata));
	com_uart_write(senddata, (7+len));
#elif(comm_mode == 3)//USE_0XFF
	senddata[0] = 'A';
	senddata[1] = 'T';
	senddata[2] = len;///'#';
	memcpy(&senddata[3],CmdStr,len);

    senddata[len+3] = 0xFF;
   com_uart_write(senddata, (4+len));
   __msg("======send cmd:%s,len:%d=======\n",senddata,(4+len));
#endif

	return;
}
///////一次性发出电话号码。
__s32 bt_phone_call(char *phonenumber,__s32 len)
{
	/////AT#CW 18664959468 \r\n
	char call_buf[BT_TEXT_LEN]={"CWN"};///后面跟号码个数
	__s32 i=0;


	__msg("===call len:%x===\n",len);
	eLIBs_memset(call_buf, 0, BT_TEXT_LEN);
	call_buf[0] = 'C';
	call_buf[1] = 'W';
	call_buf[2] = len;
	for(i=0;i<len;i++)
	{
		call_buf[3+i]=phonenumber[i];
	}
	__inf("-call-\n");
	for(i=0;call_buf[i]!=0;i++)
	{
		__inf("-%x-",call_buf[i]);
	}
	__inf("\n");
	__msg("======phone call cmd len:%d=====\n",len+3);
	BtCmdSend (call_buf,(len+3));
	////////////////////////////////////callphone 2016-01-15//////////////////////////////////
	//bt_state =BT_PHONE_OUTING;
	////////////////////////////////////////////////////////////////////////////////////////
	return EPDK_OK;
}
///////每次只能拔一位分机号。
__s32 bt_callphone_subnum(char *num)
{
	/////AT#CW 18664959468 -601 \r\n
	/////AT#CX6\r\n
	/////AT#CX0\r\n
	/////AT#CX1\r\n
	char str_buf[BT_TEXT_LEN]={"CX"};

	str_buf[2] = *num;
	__msg("======phone number:%x=======\n",num);
	BtCmdSend (str_buf,3);
	return EPDK_OK;
}
///////进入配对
__s32 bt_makepair(void)
{
	/////AT#ca\r\n
	char pair[2]={'C','A'};
	__msg("======make pair=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}
///////取消配对
__s32 bt_makeunpair(void)
{
	/////AT#cb\r\n
	char pair[2]={'C','B'};
	//__msg("======make unpair=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}
///////来电接听
__s32 bt_phone_incoming(void)
{
	/////AT#ce\r\n
	char pair[2]={'C','E'};
	//__msg("======incoming=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}
///////拒接来电
__s32 bt_phone_reject(void)
{
	/////AT#cf\r\n
	char pair[2]={'C','F'};
	//__msg("======incoming=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}
///////挂断电话
__s32 bt_phone_break(void)
{
	/////AT#cg\r\n
	char pair[2]={'C','G'};
	//__msg("======phone break=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}
///////重播电话
__s32 bt_phone_recall(void)
{
	/////AT#ch\r\n
	char pair[2]={'C','H'};
	//__msg("======recall=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}
///////vol+
__s32 bt_vol_add(void)
{
	/////AT#ck\r\n
	char pair[2]={'C','K'};
	//__msg("======vol+=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}
///////vol-
__s32 bt_vol_dec(void)
{
	/////AT#cl\r\n
	char pair[2]={'C','L'};
	//__msg("======vol-=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}
///////mute
__s32 bt_vol_mute(void)
{
	///////////////////////////////不让蓝牙芯片出声音//////////////
	/////AT#cm\r\n
	char pair[2]={'A','C'};
	//__msg("======bt_vol_mute=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}
__s32 bt_vol_unmute(void)
{	///////////////////////////////让蓝牙芯片出声音//////////////
	/////AT#cm\r\n
	char pair[2]={'A','O'};
	//__msg("======bt_vol_unmute=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}

///通知bt 关机。
__s32 bt_power_off(void)
{
	char pair[2]={'S','D'};//shut down
	__msg("======bt_shutdown=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}
__s32 bt_time_get(void)
{
	char pair[2]={'G','T'};
	__msg("======bt_time_get======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}
__s32 bt_time_set(void)
{
	/////AT#co\r\n
	char pair[12]={'S','T'};
	__msg("=====bt_time_set====\n");
	pair[0]='S';
	pair[1]='T';
	pair[2]=bt_time.year;
	pair[3]=bt_time.month;
	pair[4]=bt_time.day;
	pair[5]=bt_time.hour;
	pair[6]=bt_time.minute;
	pair[7]=0;
	BtCmdSend (pair,8);
	return EPDK_OK;
}
__s32 bt_ill_get(void)
{
	/////AT#GI\r\n
	char pair[2]={'G','I'};
	__msg("======GET ill=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}

__s32 bt_state_get(void)
{
	/////AT#GS\r\n
	char pair[2]={'G','S'};
	__msg("======GET STATE=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}

__s32 bt_phonename_get(void)
{
	/////AT#GN\r\n
	char pair[2]={'G','N'};
	__msg("======GET PHONENAME=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}

__s32 bt_hostname_get(void)
{
	/////AT#co\r\n
	char pair[2]={'G','B'};
	__msg("======GET HOST NAME=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}

__s32 bt_pincard_get(void)
{
	/////AT#GP\r\n
	char pair[2]={'G','P'};
	__msg("======GET pin card=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}

__s32 bt_macaddr_get(void)
{
	/////AT#co\r\n
	char pair[2]={'G','H'};
	__msg("======GET MAC addr=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}
__s32  bt_is_connected(void)
{
	if(bt_connect_flag	 == BT_STATE_CONNECTED)
		return 1;
	else
		return 0;
}
__s32 bt_connect_set(void)
{
	/////AT#co\r\n
	char pair[2]={'G','R'};
	__msg("======set bt connect=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}

__s32 bt_disconnect_set(void)
{
	/////AT#co\r\n
	char pair[2]={'G','D'};
	__msg("======set bt disconnect=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}

///////HFP状态查询
////////返回:MG<参数> or  ERROR
/////////////////<参数> :0-> ready
/////////////////<参数> :1-> connected
/////////////////<参数> :2-> outgoing call
/////////////////<参数> :3-> incoming call
/////////////////<参数> :4-> active call
__s32 bt_hfpstate_get(void)
{
	/////AT#cy\r\n
	char pair[3]={'C','Y'};
	__msg("======bt_get_hfpstate=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}

///////audio play/pause
__s32 bt_audio_play_pause(void)
{
	/////AT#MA\r\n
	char pair[2]={'M','A'};
	__msg("======bt_audio_play_pause=======\n");
	BtCmdSend (pair,2);
#if 0
    if(BT_STAT_PLAY == bt_play_status)
    {
        bt_play_status = BT_STAT_PAUSE;
        __wrn("BT_STAT_PLAY [%d]",bt_play_status);
    }
    else if(BT_STAT_PAUSE == bt_play_status)
    {
        bt_play_status = BT_STAT_PLAY;
        __wrn("BT_STAT_PLAY [%d]",bt_play_status);
    }
    else
    {
        bt_play_status = BT_STAT_PLAY;
        __wrn("bt_play_status first?? [%d]",bt_play_status);
    }
 #endif
	return EPDK_OK;
}
///////audia stop
__s32 bt_audio_play(void)
{
	/////AT#MC\r\n
	char pair[2]={'M','Y'};
	__msg("======bt_audio_play=======\n");
	BtCmdSend (pair,2);

	return EPDK_OK;
}

///////audia stop
__s32 bt_audio_pause(void)
{
	/////AT#MC\r\n
	char pair[2]={'M','S'};
	__msg("======bt_audio_pause=======\n");
	BtCmdSend (pair,2);

	return EPDK_OK;
}
///////audia stop
__s32 bt_audio_stop(void)
{
	/////AT#MC\r\n
	char pair[2]={'M','C'};
	__msg("======bt_audio_stop=======\n");
	BtCmdSend (pair,2);

	return EPDK_OK;
}
__s32 bt_audio_prev(void)
{
	/////AT#ME\r\n
	char pair[2]={'M','E'};
	__msg("======channel change=======\n");
	BtCmdSend (pair,2);

	return EPDK_OK;
}
///////audia next
__s32 bt_audio_next(void)
{
	/////AT#MD\r\n
	char pair[2]={'M','D'};
	__msg("======bt_audio_next=======\n");
	BtCmdSend (pair,2);

	return EPDK_OK;
}
///////audio get music status/
__s32 bt_audio_get_status(void)
{
    return bt_play_status;
}
///////audio get music status/
__s32 bt_audio_set_status(uint8_t play_status)
{
     bt_play_status = play_status;
     return EPDK_OK;
}
///////audia next
__s32 bt_phonebook_get(void)
{
	/////AT#MD\r\n
	char pair[2]={'B','A'};
	__msg("======bt_phonebook_get=======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}
///////audia channel change
__s32 bt_audio_change(void)
{
	/////AT#CO  or  CP\r\n
	char pair[2]={'C','O'};
	__msg("======bt_audio_change======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}
///////audio change///强行切到手机上
__s32 bt_audio_to_phone(void)
{
	/////AT#CO  or  CP\r\n
	char pair[2]={'C','P'};
	__msg("======bt_audio_to_phone======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}
///////audio change///强行切到车机上
__s32 bt_audio_to_machine(void)
{
	/////AT#CO  or  CP\r\n
	char pair[2]={'C','M'};
	__msg("======bt_audio_to_machine======\n");
	BtCmdSend (pair,2);
	return EPDK_OK;
}

__s32 bt_eq_user_mode(__u8 index, __u8 gain)
{
	char buf[4]={'\0'};
	buf[0]= 'E';
	buf[1]= 'U';
	buf[2]= index;
	buf[3]= gain;
	__msg("======bt_eq_user_mode======\n");
	BtCmdSend (buf,4);
	return EPDK_OK;
}

__s32 bt_eq_pop_mode(__u8 index, __u8 gain)
{
	char buf[4]={'\0'};
	buf[0]= 'E';
	buf[1]= 'P';
	buf[2]= index;
	buf[3]= gain;
	__msg("======bt_eq_pop_mode======\n");
	BtCmdSend (buf,4);
	return EPDK_OK;
}

__s32 bt_eq_rock_mode(__u8 index, __u8 gain)
{
	char buf[4]={'\0'};
	buf[0]= 'E';
	buf[1]= 'R';
	buf[2]= index;
	buf[3]= gain;
	__msg("======bt_eq_rock_mode======\n");
	BtCmdSend (buf,4);
	return EPDK_OK;
}

__s32 bt_eq_jazz_mode(__u8 index, __u8 gain)
{
	char buf[4]={'\0'};
	buf[0]= 'E';
	buf[1]= 'J';
	buf[2]= index;
	buf[3]= gain;
	__msg("======bt_eq_jazz_mode======\n");
	BtCmdSend (buf,4);
	return EPDK_OK;
}

__s32 bt_eq_classic_mode(__u8 index, __u8 gain)
{
	char buf[4]={'\0'};
	buf[0]= 'E';
	buf[1]= 'C';
	buf[2]= index;
	buf[3]= gain;
	__msg("======bt_eq_classic_mode======\n");
	BtCmdSend (buf,4);
	return EPDK_OK;
}

__s32 bt_eq_gentle_mode(__u8 index, __u8 gain)
{
	char buf[4]={'\0'};
	buf[0]= 'E';
	buf[1]= 'G';
	buf[2]= index;
	buf[3]= gain;
	__msg("======bt_eq_gentle_mode======\n");
	BtCmdSend (buf,4);
	return EPDK_OK;
}

__s32 bt_dac_vol_front(__u8 vol)
{
	char buf[3]={'\0'};
	buf[0]= 'D';
	buf[1]= 'F';
	buf[2]= vol;
	__msg("======bt_dac_vol_front======\n");
	BtCmdSend (buf,3);
	return EPDK_OK;
}

__s32 bt_dac_vol_behind(__u8 vol)
{
	char buf[3]={'\0'};
	buf[0]= 'D';
	buf[1]= 'B';
	buf[2]= vol;
	__msg("======bt_dac_vol_behind======\n");
	BtCmdSend (buf,3);
	return EPDK_OK;
}

__s32 bt_dac_vol_left(__u8 vol)
{
	char buf[3]={'\0'};
	buf[0]= 'D';
	buf[1]= 'L';
	buf[2]= vol;
	__msg("======bt_dac_vol_left======\n");
	BtCmdSend (buf,3);
	return EPDK_OK;
}

__s32 bt_dac_vol_right(__u8 vol)
{
	char buf[3]={'\0'};
	buf[0]= 'D';
	buf[1]= 'R';
	buf[2]= vol;
	__msg("======bt_dac_vol_right======\n");
	BtCmdSend (buf,3);
	return EPDK_OK;
}
/*******************SPP&BLE COMM MODE必须采用1或者2通讯方式**********************/

/****************************spp comm********************************************/

//"AT#"打头的指令透传通道

__s32 bt_spp_tx_data(char *CmdStr,unsigned char len)
{
    char *tmpCmdStr = NULL;
    tmpCmdStr = strstr(CmdStr,"AT#");
    //__wrn("==2==bt_spp_tx cmd:%s ,str_len[%d],len:%d===\n",tmpCmdStr,strlen(tmpCmdStr),tmpCmdStr-CmdStr);
    memset(spp_txdata,0,sizeof(spp_txdata));
    spp_txdata[0] = 'A';
    spp_txdata[1] = 'T';
    spp_txdata[2] = '#';
    spp_txdata[3] = '#';
    memcpy(&spp_txdata[4],tmpCmdStr+3,len-(tmpCmdStr-CmdStr));

#if(comm_mode ==1)//AT##比AT#多1
    spp_txdata[len+4-3] = '\r';
    spp_txdata[len+5-3] = '\n';
    __wrn("==3==bt_spp_tx cmd:%s ,str_len[%d],len:%d====\n",spp_txdata,strlen(spp_txdata),len+3);
    com_uart_write(spp_txdata, strlen(CmdStr)+3);
#elif (comm_mode ==2)
    spp_txdata[len+1] = '\\';
    spp_txdata[len+2] = 'r';
    spp_txdata[len+3] = '\\';
    spp_txdata[len+4] = 'n';
    __wrn("==3==bt_spp_tx cmd:%s ,str_len[%d],len:%d====\n",spp_txdata,strlen(spp_txdata),len+5);
    com_uart_write(spp_txdata, strlen(CmdStr)+5);
#endif

	return EPDK_OK;
}

//请求蓝牙版本号
__s32 bt_spp_request_bt_version(void)
{
	char buf[5]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'Q';
	buf[4]= 'V';
	__msg("======bt_spp_request_bt_version======\n");
	bt_spp_tx_data (buf,5);
	return EPDK_OK;
}
//请求蓝牙信息，返回的是手机 name ,mac addr,bt uuid
__s32 bt_spp_request_bt_info(void)
{
	char buf[5]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'Q';
	buf[4]= 'I';
	__msg("======bt_spp_request_bt_info======\n");
	bt_spp_tx_data (buf,5);
	return EPDK_OK;
}
__s32 bt_spp_rfcomm_is_connected(void)
{
    if(bt_spp_connect_flag == BT_SPP_STATE_CONNECTED)
		return 1;
	else
		return 0;

	return EPDK_OK;
}

__s32 bt_spp_rfcomm_connect(void)
{
	char buf[5]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'S';
	buf[4]= 'P';
	__msg("======bt_spp_connect======\n");
	bt_spp_tx_data (buf,5);
	return EPDK_OK;
}
__s32 bt_spp_rfcomm_disconnect(void)
{
	char buf[5]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'S';
	buf[4]= 'H';
	__msg("======bt_spp_disconnect======\n");
	bt_spp_tx_data (buf,5);
	return EPDK_OK;
}


//RFCOMM连接成功后，AT#SG加上第三方手机互联库发过来的形成数据串，转为AT##再通过RFCOMM发送给手机
//libzbt_rfcomm_data_send
__s32 bt_spp_tx_data_rfcomm(char *CmdStr,unsigned char len)
{
    memset(tmp_spp_txdata,0,sizeof(tmp_spp_txdata));
	tmp_spp_txdata[0]= 'A';
	tmp_spp_txdata[1]= 'T';
	tmp_spp_txdata[2]= '#';
	tmp_spp_txdata[3]= 'S';
	tmp_spp_txdata[4]= 'G';
	memcpy(&tmp_spp_txdata[5],CmdStr,len);
    bt_spp_tx_data(tmp_spp_txdata,len+5);
	return EPDK_OK;
}

//RFCOMM连接成功后，手机上层发过来的RFCOMM数据串，后面可发送第三方手机互联库
//libzbt_rfcomm_data_recv_CB_init
__s32 bt_spp_rx_data_rfcomm(char *CmdStr,unsigned char len)
{
    memset(spp_rxdata,0,sizeof(spp_rxdata));
	memcpy(&spp_rxdata,CmdStr,len);
	return EPDK_OK;
}

/****************************ble comm************************************/
#if 0
//"AT#"打头的指令透传通道

__s32 bt_ble_tx_data(char *CmdStr,unsigned char len)
{
    char *tmpCmdStr = NULL;
    tmpCmdStr = strstr(CmdStr,"AT#");
    //__wrn("==2==bt_spp_tx cmd:%s ,str_len[%d],len:%d===\n",tmpCmdStr,strlen(tmpCmdStr),tmpCmdStr-CmdStr);
    memset(spp_txdata,0,sizeof(spp_txdata));
    spp_txdata[0] = 'A';
    spp_txdata[1] = 'T';
    spp_txdata[2] = '#';
    spp_txdata[3] = '*';
    memcpy(&spp_txdata[4],tmpCmdStr+3,len-(tmpCmdStr-CmdStr));

#if(comm_mode ==1)//AT#*比AT#多1
    spp_txdata[len+4-3] = '\r';
    spp_txdata[len+5-3] = '\n';
    __wrn("==3==bt_spp_tx cmd:%s ,str_len[%d],len:%d====\n",spp_txdata,strlen(spp_txdata),len+3);
    com_uart_write(spp_txdata, strlen(CmdStr)+3);
#elif (comm_mode ==2)
    spp_txdata[len+1] = '\\';
    spp_txdata[len+2] = 'r';
    spp_txdata[len+3] = '\\';
    spp_txdata[len+4] = 'n';
    __wrn("==3==bt_spp_tx cmd:%s ,str_len[%d],len:%d====\n",spp_txdata,strlen(spp_txdata),len+5);
    com_uart_write(spp_txdata, strlen(CmdStr)+5);
#endif

	return EPDK_OK;
}
__s32 bt_ble_rfcomm_is_connected(void)
{
    if(bt_ble_connect_flag	 == BT_BLE_STATE_CONNECTED)
		return 1;
	else
		return 0;

	return EPDK_OK;
}
__s32 bt_ble_rfcomm_reconnect(void)
{
	char buf[5]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'S';
	buf[4]= 'P';
	__msg("======bt_ble_connect======\n");
	bt_ble_tx_data (buf,5);
	return EPDK_OK;
}
__s32 bt_ble_rfcomm_disconnect(void)
{
	char buf[5]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'S';
	buf[4]= 'H';
	__msg("======bt_ble_disconnect======\n");
	bt_ble_tx_data (buf,5);
	return EPDK_OK;
}

//RFCOMM连接成功之后，上层发过来的是AT#开头的数据串，转为AT#*
__s32 bt_ble_tx_data_rfcomm(char *CmdStr,unsigned char len)
{

    bt_ble_tx_data(CmdStr,len);
	return EPDK_OK;
}
#endif

//bt hid api function

__s32 bt_hid_tx_data(char *CmdStr,unsigned char len)
{
    char *tmpCmdStr = NULL;
    tmpCmdStr = strstr(CmdStr,"AT#");
    //__wrn("==2==bt_spp_tx cmd:%s ,str_len[%d],len:%d===\n",tmpCmdStr,strlen(tmpCmdStr),tmpCmdStr-CmdStr);
    memset(spp_txdata,0,sizeof(spp_txdata));
    spp_txdata[0] = 'A';
    spp_txdata[1] = 'T';
    spp_txdata[2] = '#';
    spp_txdata[3] = '@';
    memcpy(&spp_txdata[4],tmpCmdStr+3,len-(tmpCmdStr-CmdStr));

#if(comm_mode ==1)//AT#@比AT#多1
    spp_txdata[len+4-3] = '\r';
    spp_txdata[len+5-3] = '\n';
    //__wrn("==3==bt_spp_tx cmd:%s ,str_len[%d],len:%d====\n",spp_txdata,strlen(spp_txdata),len+3);
    //com_uart_write(spp_txdata, strlen(CmdStr)+3);
    com_uart_write(spp_txdata, len+3);
#elif (comm_mode ==2)
    spp_txdata[len+1] = '\\';
    spp_txdata[len+2] = 'r';
    spp_txdata[len+3] = '\\';
    spp_txdata[len+4] = 'n';
    __wrn("==3==bt_spp_tx cmd:%s ,str_len[%d],len:%d====\n",spp_txdata,strlen(spp_txdata),len+5);
    com_uart_write(spp_txdata, strlen(CmdStr)+5);
#endif

	return EPDK_OK;
}

__s32 bt_hid_is_connected(void)
{
    if(bt_ble_connect_flag == BT_HID_STATE_CONNECTED)
		return 1;
	else
		return 0;

	return EPDK_OK;
}

__s32 bt_hid_connect(void)
{
	char buf[5]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'H';
	buf[4]= 'C';
	__msg("======bt_hid_connect======\n");
	bt_hid_tx_data (buf,5);
	return EPDK_OK;
}

__s32 bt_hid_disconnect(void)
{
	char buf[5]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'H';
	buf[4]= 'D';
	__msg("======bt_hid_disconnect======\n");
	bt_hid_tx_data (buf,5);
	return EPDK_OK;
}
//设置手机光标指针的坐标位置给蓝牙
__s32 bt_hid_set_cursor_pos(s16 x_pos,s16 y_pos)
{
	char buf[10]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'H';
	buf[4]= 'S';
	buf[5]= x_pos&0xff;
	buf[6]= (x_pos&0xff00)>>8;
	buf[7]= y_pos&0xff;
	buf[8]= (y_pos&0xff00)>>8;
	__msg("======bt_hid_set_cursor_pos======\n");
	bt_hid_tx_data (buf,9);
	return EPDK_OK;
}
//发送触摸滑动点坐标相对变化值(需要减去上一次坐标值)给bt
//action 0:指针移动；1:鼠标左键按下；2:鼠标左键抬起；3:鼠标中键按下；4:鼠标中键抬起；
//5:鼠标右键按下；6:鼠标右键抬起；
__s32 bt_hid_set_cursor_xy(u8 action,s16 x_diff,s16 y_diff)
{
	char buf[10]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'H';
	buf[4]= 'O';

	buf[5]= action;
	buf[6]= x_diff&0xff;
	buf[7]= (x_diff&0xff00)>>8;
	buf[8]= y_diff&0xff;
	buf[9]= (y_diff&0xff00)>>8;
	__msg("======bt_hid_set_cursor_xy======\n");
	bt_hid_tx_data (buf,10);
	return EPDK_OK;
}
//重置手机光标指针的坐标位置给蓝牙，这个时候的光标位置就是bt_hid_set_cursor_pos函数设置的坐标

__s32 bt_hid_reset_cursor_pos(void)
{
	char buf[5]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'H';
	buf[4]= 'P';

	__msg("======bt_hid_reset_cursor_pos======\n");
	bt_hid_tx_data (buf,5);
	return EPDK_OK;
}

__s32 bt_hid_mouse_test(void)
{
	char buf[5]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'H';
	buf[4]= 'T';
    __msg("======bt_hid_mouse_test======\n");
	bt_hid_tx_data (buf,5);
	return EPDK_OK;

}

//bt fm api function
/****************************fmrx comm********************************************/

//搜台
__s32 bt_fmrx_search (void)
{
    char buf[5]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'F';
	buf[4]= 'S';
	__msg("======bt_fmrx_search======\n");
	BtCmdSend (buf,5);
	return EPDK_OK;

}
//获取节目频道号信息，如当前频点，总节目数等
__s32 bt_fmrx_get_info (void)
{
    char buf[5]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'F';
	buf[4]= 'I';
	__msg("======bt_fmrx_get_info======\n");
	BtCmdSend (buf,5);
	return EPDK_OK;
}
//获取节目频道列表等
__s32 bt_fmrx_get_list (void)
{
    char buf[5]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'F';
	buf[4]= 'L';
	__msg("======bt_fmrx_get_list======\n");
	BtCmdSend (buf,5);
	return EPDK_OK;
}
//获取总节目数
__s32 bt_fmrx_get_ch_cnt (void)
{
    char buf[5]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'F';
	buf[4]= 'T';
	__msg("======bt_fmrx_get_ch_cnt======\n");
	BtCmdSend (buf,5);
	return EPDK_OK;
}
//播放节目号

__s32 bt_fmrx_set_ch (__u8 ch)
{
    char buf[6]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'F';
	buf[4]= 'C';
	buf[5]= ch;
	__msg("======bt_fmrx_set_ch======\n");
	BtCmdSend (buf,6);
	return EPDK_OK;
}
//播放频点
__s32 bt_fmrx_set_freq (__u16 freq)
{
    char buf[7]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'F';
	buf[4]= 'F';
	buf[5]= freq&0xFF;
	buf[6]= (freq>>8)&0xFF;
	__msg("======bt_fmrx_set_freq======\n");
	BtCmdSend (buf,7);
	return EPDK_OK;
}
//切节目号，dir为1往108M方向切，为0往78.5M方向切
__s32 bt_fmrx_switch_channel (__s8 dir)
{
    char buf[6]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'F';
	buf[4]= 'N';
	buf[5]= dir;
	__msg("======bt_fmrx_switch_channel======\n");
	BtCmdSend (buf,6);
	return EPDK_OK;
}
//切频点，dir为1往108M方向切，为0往78.5M方向切
__s32 bt_fmrx_switch_freq (__s8 dir)
{
    char buf[6]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'F';
	buf[4]= 'R';
	buf[5]= dir;
	__msg("======bt_fmrx_switch_freq======\n");
	BtCmdSend (buf,6);
	return EPDK_OK;
}

//获取节目号对应的频点
__s32 bt_fmrx_get_ch_to_freq (__u8 ch)
{
    char buf[6]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'F';
	buf[4]= 'D';
	buf[5]= ch;
	__msg("======bt_fmrx_switch_freq======\n");
	BtCmdSend (buf,6);
	return EPDK_OK;
}
//切换蓝牙的应用,BT启动默认蓝牙应用//
//index值如下:
//0x41: FMRX;   0x42:AUX2;   0x43:BT;    0x44:MUSIC
__s32 bt_switch_app(u8 index)
{
    char buf[6]={'\0'};
	buf[0]= 'A';
	buf[1]= 'T';
	buf[2]= '#';
	buf[3]= 'C';
	buf[4]= 'O';
	buf[5]= index;
	__msg("======bt_fmrx_switch_freq======\n");
	BtCmdSend (buf,6);


}

void bt_StrToHex(unsigned char *pbDest, unsigned char *pbSrc, int srcLen)
{
    char h1,h2;
    unsigned char s1,s2;
    int i;
    for (i=0; i<srcLen; i++)
    {
        h1 = pbSrc[2*i];
        h2 = pbSrc[2*i+1];
        s1 = eLIBs_toupper(h1) - 0x30; //十六进制 0x30, dec十进制 48	,   图形 0
        if (s1 > 9)
            s1 -= 7;
        s2 = eLIBs_toupper(h2) - 0x30;
        if (s2 > 9)
            s2 -= 7;
        pbDest[i] = s1*16 + s2;
    }
    return;
}

void bt_HexToStr(char *pszDest, char *pbSrc, int srcLen)
{
    char    ddl, ddh;
    for (int i = 0; i < srcLen; i++)
    {
        ddh = 48 + pbSrc[i] / 16;
        ddl = 48 + pbSrc[i] % 16;
        if (ddh > 57) ddh = ddh + 7;
        if (ddl > 57) ddl = ddl + 7;
        pszDest[i * 2] = ddh;
        pszDest[i * 2 + 1] = ddl;
    }
    pszDest[srcLen * 2] = '\0';
    return;
}

#ifdef __cplusplus
}
#endif

