/**************************************************************************************************************
 * altobeam RTOS WSM host interface (HI) implementation
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

  

#include "atbm_hal.h"
extern struct atbmwifi_common g_hw_prv;
static atbm_uint32    smt_magic_channel_mask= 0;
int free_legacy_data(void);
atbm_int32 check_smt_rx_done(void);
extern atbm_void AT_WDisConnect_vif(struct atbmwifi_vif *priv,char *pLine);
atbm_void atbmwifi_stop_smartconfig(struct atbmwifi_vif *hw_priv );
 atbm_void smartconfig_start_timer_func(atbm_void *data1,atbm_void *data2);
atbm_int32 smartconfig_may_magic_channel(atbm_int32 channel)
{
	return  smt_magic_channel_mask&BIT(channel);
}


atbm_int32 smartconfig_have_may_magic_channel(void)
{
	return  smt_magic_channel_mask;
}

atbm_void smartconfig_clear_magic_channel(void)
{
	smt_magic_channel_mask=0;
}

atbm_void smartconfig_set_magic_channel(atbm_int32 channel)
{
	smt_magic_channel_mask|=BIT(channel);
}
 atbm_void smartconfig_scan_start(struct atbmwifi_vif *priv)
{
	priv->scan_ret.len = 0;	
	priv->scan_expire = 2;
	priv->scan_no_connect_back = 1;
	priv->scan_no_connect = 1;
	/*start smartconfig*/
	priv->scan.scan_smartconfig = 1;
	priv->st_status = CONFIG_ST_START;

	atbmwifi_scan_start(priv);
	priv->st_status = CONFIG_ST_GET_MAGIC;
}

 atbm_int32 smartconfig_start(struct smartconfig_config * st_cfg,int if_id)
{	
	struct atbmwifi_vif *priv;
	atbm_int32 Istate=0;
	priv= _atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,if_id);
	if(st_cfg == ATBM_NULL){
		wifi_printk(WIFI_ALWAYS,"unsupport  st_cfg ATBM_NULL!");
		return CONFIG_ST_STARTCONFIG_ERR;
	}
	if((st_cfg->magic_cnt<1)||(st_cfg->magic_cnt>10)){
		wifi_printk(WIFI_ALWAYS,"unsupport  imagic_cnt %d!",st_cfg->magic_cnt);
		return CONFIG_ST_STARTCONFIG_ERR;
	}
	if((st_cfg->magic_time<20)||(st_cfg->magic_time>500)){
		wifi_printk(WIFI_ALWAYS,"unsupport  magic_time!");
		return CONFIG_ST_STARTCONFIG_ERR;
	}
	if((st_cfg->payload_time<500)||(st_cfg->payload_time>30000)){
		wifi_printk(WIFI_ALWAYS,"unsupport  payload_time!");
		return CONFIG_ST_STARTCONFIG_ERR;
	}
	 atbm_memcpy(&priv->st_cfg,st_cfg,sizeof(struct smartconfig_config ));
	 /*disconnect AP*/
#if ATBM_PLATFORM == AK_RTOS_37D
	extern int msh_exec(char *cmd, rt_size_t length);
	char cmd[10];
	strcpy(cmd, "wifi disc");
	msh_exec(cmd, strlen(cmd));
#else
	 AT_WDisConnect(ATBM_NULL);
#endif
	 atbmwifi_eloop_register_timeout(0,13 * (st_cfg->payload_time + st_cfg->magic_time*14),smartconfig_start_timer_func,(atbm_void *)priv,ATBM_NULL);

	 if(priv->scan_ret.info){
		Istate = atbm_local_irq_save();
		atbm_kfree(priv->scan_ret.info);
		priv->scan_ret.info = ATBM_NULL;
		priv->scan_ret.len =0;
		atbm_local_irq_restore(Istate);
	 }	
	 smartconfig_scan_start(priv);
	 return CONFIG_ST_START;
}

 atbm_int32 smartconfig_stop(int if_id)
{
	struct atbmwifi_vif *priv;
	priv= _atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,if_id);
	atbmwifi_stop_smartconfig(priv);
	return priv->st_status;
}
 atbm_int32 smartconfig_status(int if_id)
{
	struct atbmwifi_vif *priv;
	priv= _atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,if_id);
	return priv->st_status;
}

/*cnt : 2 ~ 100*/
atbm_int32 smartconfig_magic_channel_cnt(atbm_int32 cnt)
{
	return 0;
}

/*ms : 20ms ~ 1000ms*/
atbm_int32 smartconfig_magic_channel_timeout(atbm_int32 ms)
{
	return 0;
}
/*ms : 500ms ~ 10000ms*/
atbm_int32 smartconfig_payload_timeout(atbm_int32 ms)
{
	return 0;
}
 atbm_void atbmwifi_stop_smartconfig(struct atbmwifi_vif *priv )
{
	struct atbmwifi_common *hw_priv;
	struct wsm_reset reset = { 0 };
	hw_priv=_atbmwifi_vifpriv_to_hwpriv(priv);

	priv->st_status = CONFIG_ST_IDLE;
	priv->scan.scan_smartconfig = 0;
	reset.reset_statistics = ATBM_TRUE;
	atbmwifi_eloop_cancel_timeout(smartconfig_start_timer_func, (atbm_void *)priv, ATBM_NULL);
	if(priv->scan.in_progress){
		wsm_stop_scan(hw_priv,0);
		atbmwifi_scan_comlete(priv);
	}

	free_legacy_data();
	reset.link_id = 0;
	wsm_reset(hw_priv,&reset,0);
}

 atbm_void smartconfig_start_timer_func(atbm_void *data1,atbm_void *data2)
{		
	struct atbmwifi_vif *priv=(struct atbmwifi_vif *)data1;
	if(priv->scan.scan_smartconfig){
		atbmwifi_stop_smartconfig(priv);
		priv->st_status = CONFIG_ST_DONE_FAIL;
	}
}

 atbm_int32 smartconfig_magic_scan_done(struct atbmwifi_vif *priv)
{
	struct atbmwifi_common *hw_priv;
	hw_priv=_atbmwifi_vifpriv_to_hwpriv(priv);
	if(priv->st_status == CONFIG_ST_SWITCH_PAYLOAD_CH ){
		struct wsm_set_chantype arg ;
		{
			arg.band = 0;			//0:2.4G,1:5G
			arg.flag = 0;			//no use
			arg.channelNumber = priv->st_configchannel;	// channel number
			arg.channelType = CH_OFF_20;	// channel type
		};
		arg.flag |= BIT(4);
		wifi_printk(WIFI_ALWAYS,"st_configchannel %d\n",priv->st_configchannel);
		wsm_set_chantype_func(hw_priv,&arg,0);
		priv->st_status = CONFIG_ST_GET_PAYLOAD;
	}
	else {
		if(priv->st_status == CONFIG_ST_GET_MAGIC){
			//may need start scan again
			smartconfig_scan_start(priv);
	    }
	}

	smartconfig_clear_magic_channel();
	return 0;
}

/*********************************************
start of new smartconfig RX code
*******************************************/

#define verbose_debug 0

#define ATBM_CONFIG_ST_GET_MAGIC   1 
#define ATBM_CONFIG_ST_GET_PAYLOAD 2

#define HT_PAYLOAD_EXTR_LENGTH  288  //0x120, 避开控制包密集区
#define HT_PAYLOAD_MAX_LENGTH   1398 //0x120 + 0x400 + 82, 避开控制包密集区

#define MIN_GROUP_DATA_COUNTER 8    //typically, 72 packet one group, at least receive 30%  packets(include interference packets).

#define MIN_MAC_BASE_LEN  64
#define MAX_MAC_BASE_LEN  86

#define MAX_MIN_MAGIC_PAIR_LEN  123  //0x3f + 60
#define MAX_MAC_MAGIC_PAIR_LEN  312  //0xe2 + 86

static const atbm_uint8  smt_mpdu_base_length[] = {60, 62, 68, 70, 76, 78, 80, 82}; //
//const static atbm_uint8  ht_mpdu_base_length[] = {64, 66, 72, 74, 80, 82, 84, 86}; //msdu +  mac length. + FCS


//5e bc e2 61 3f dd 83 c2 9c 7e 20 a3 fd 1f 41 9d 
static const atbm_uint8 smt_mpdu_pair_length[20] = {0x00, 0x5e, 0xbc, 0xe2, 0x61,0x3f, 0xdd,
	0x83, 0xc2, 0x9c, 0x7e,0x20, 0xa3,
	0xfd, 0x1f, 0x41, 0x9d };  //提前计算好crc8 of index value， 接收时减少计算


static atbm_int32 ht_state;
#define HT_CONFIG_ST_GET_MAGIC   1 
#define HT_CONFIG_ST_GET_PAYLOAD 2
#define HT_CONFIG_ST_GET_DONE   3
//static int 	  atbm_smt_state = ATBM_CONFIG_ST_GET_MAGIC;

#define SMART_TYPE_CNT 2

enum SMART_TYPE {
	ADDR1_TYPE=0,
	ADDR3_TYPE=1,
};

struct base_dec_t{
	int group_idx;
	int len;
	int group_counter;
};

struct base_dec_l{
	struct base_dec_t base_sec[10][SMART_TYPE_CNT];
	int counter[SMART_TYPE_CNT];
	int satisfied;
	int received;
};

struct base_dec_l base_sec_l;
static atbm_uint8  base_mac_len[SMART_TYPE_CNT]  = {0};         /*保存基准长度*/
static atbm_uint16 pre_pair_data[2] = {0};         /*保存CRC配对数据*/
static atbm_int16 fromds_channel_rssi[14];
static atbm_int16 fromds_channel_paired_counter[14];

static atbm_uint8 fromds_channel = 15; //invalid value
static atbm_uint8 fromds_magic_mask;   //bit0, ,1,2,3,4
static atbm_int32 fromds_magic_counter; 
static atbm_uint8 cur_group_index = 0;       /*保存当前接收的组号*/
static atbm_uint8 payload_total_length = 0;
static atbm_uint8 payload_total_xor = 0;
static atbm_uint8 payload_pwd_length = 0;
static atbm_uint8 smt_payload_data[128];
static atbm_uint8 smt_ssid[32];
static atbm_uint8 smt_pwd[64];
static atbm_uint16 group_received_mask = 0;

static atbm_uint8 group_payload[8];             //动态接收的组数据,如果通过CRC8校验，存入smt_payload_data区
static atbm_uint8 group_data_cur_counter = 0;  //记录每组数据接收到的包数，CRC计算要求至少收到一定比例的包，例如30%; 防止包接收的比较少，无统计信息。

//static atbm_uint8 group_replace[8];            //ht40_group_data使用的replace方式，如果数据收到的次数 > 1 不replace， 否则，依次replace次数为1的包， 防止误包占满组数据buffer
static atbm_uint8 group_data_mask[15];         //掩码位，CRC计算要求当前组每个索引位置数据都收到
static atbm_uint16 group_mask = 0;
/*
用四组数保存可能的数据
第一组数保存概率最多的数
第二组之后保存动态刷新的数
*/
//will sort and find the maximum received data.
struct ht40_smt_data
{
	atbm_uint8 rdata[4];
	unsigned short rdata_counter[4];
};
static struct ht40_smt_data ht40_group_data[8];  


//保存历史数据， 合并新的数据，用于积累组数据
struct ht40_smart_legacy_data_struct
{
	atbm_uint8 groups_num;           //the counter of group data
	atbm_uint8 legacy_depth;         //saved how many round data.
	atbm_uint8 legacy_policy;        //update policy.
	atbm_uint8 reserved;
	atbm_uint8 *legacy_data_round;   //round of legacy data.
	struct ht40_smt_data *p_legacy_data;
};
static struct ht40_smart_legacy_data_struct ht40_smt_legacy_data;  

void fromap_sort_and_set_data(void);
void fromap_merge_smt_data(atbm_uint8 cur_g_indx, atbm_uint8 score_level);


/*
分配内存，用于保存配网历史数据
使用当前组 + 历史数据两种方式校验组内CRC
*/
int malloc_legacy_data(void)
{
	ht40_smt_legacy_data.p_legacy_data = (struct ht40_smt_data *)atbm_kmalloc(sizeof(struct ht40_smt_data )*128, GFP_KERNEL);
	ht40_smt_legacy_data.legacy_data_round = (atbm_uint8 *)atbm_kmalloc(sizeof(atbm_uint8)*128, GFP_KERNEL);

	if((ht40_smt_legacy_data.p_legacy_data != NULL) && (ht40_smt_legacy_data.legacy_data_round  !=NULL ))
	{
		atbm_memset(ht40_smt_legacy_data.p_legacy_data, '\0', sizeof(struct ht40_smt_data )*128);
		atbm_memset(ht40_smt_legacy_data.legacy_data_round, '\0', sizeof(atbm_uint8)*128);
		return 0;
	}
	return -1;
}

int free_legacy_data(void)
{
	if(ht40_smt_legacy_data.p_legacy_data != NULL)
	{
		atbm_kfree(ht40_smt_legacy_data.p_legacy_data);
		ht40_smt_legacy_data.p_legacy_data = NULL;
	}
	if(ht40_smt_legacy_data.legacy_data_round !=NULL)
	{
		atbm_kfree(ht40_smt_legacy_data.legacy_data_round);
		ht40_smt_legacy_data.legacy_data_round = NULL;
	}
	base_mac_len[ADDR1_TYPE] = 60;
	base_mac_len[ADDR3_TYPE] = 60;
	ht_state = HT_CONFIG_ST_GET_MAGIC;
	
	return 0;

}

void reset_group_data(atbm_uint8 group_index)
{
	atbm_memset(ht40_group_data, '\0', sizeof(ht40_group_data));
//	atbm_memset(group_replace, '\0', sizeof(group_replace));		
	group_data_cur_counter = 0;
	if(group_index < 15)
	{
		group_data_mask[group_index] = 0;
	}
	if(group_index == 0xff)
	{
		atbm_memset(group_data_mask, '\0', sizeof(group_data_mask));		
	}
}

atbm_uint8 smtcrc8(atbm_uint8 *A,atbm_uint8 n)
{
	atbm_uint8 i;
	atbm_uint8 checksum = 0;

	while(n--)
	{
		for(i=1;i!=0;i*=2)
		{
			if( (checksum&1) != 0 )
			{
				checksum /= 2;
				checksum ^= 0x8C;
			}
			else
			{
				checksum /= 2;
			}

			if( (*A & i) != 0 )
			{
				checksum ^= 0x8C;
			}
		}
		A++;
	}
	return(checksum);
}

int get_strong_channel(void)
{
	int target_ch = 0;
	int i = 0;
	short rssi = -100;
	for(i = 1; i< 14; i++)
	{     
		//printf("target_ch:%d rssi:%d cnt:%d\n", i,fromds_channel_rssi[i], fromds_channel_paired_counter[i]);
		if((rssi < fromds_channel_rssi[i])&&(fromds_channel_rssi[i] < 0))
		{
			rssi = fromds_channel_rssi[i] + fromds_channel_paired_counter[i] ;
			//printf("target_ch:%d rssi_eva:%d\n", i,rssi);
			target_ch = i;
		}
	}
	return target_ch;
}

atbm_void set_base(atbm_uint8 smt_type, int group_index, int len){
	int i;
	int flag = 0;

	wifi_printk(WIFI_ALWAYS, "smt_type:%d group_index:%d len:%d received:%d\n", smt_type, group_index, len, base_sec_l.received);
	if(base_sec_l.counter[smt_type] > 0){
		for(i = 0; i < base_sec_l.counter[smt_type]; i++){
			if(base_sec_l.base_sec[i][smt_type].len == len){
				flag = 1;
				base_mac_len[smt_type] = len;
				if(base_sec_l.received > 20){
					base_sec_l.satisfied = 1;
					break;
				}
				if(base_sec_l.base_sec[i][smt_type].group_idx != group_index){
					if(base_sec_l.received > 15 && base_mac_len[!smt_type])
					{
						base_sec_l.satisfied = 1;
					}
					break;
				}
			}
		}
		if(!flag && base_sec_l.counter[smt_type] < 10){
			base_sec_l.base_sec[base_sec_l.counter[smt_type]][smt_type].group_counter = 1;
			base_sec_l.base_sec[base_sec_l.counter[smt_type]][smt_type].group_idx = group_index;
			base_sec_l.base_sec[base_sec_l.counter[smt_type]][smt_type].len = len;
			base_sec_l.counter[smt_type]++;
		}
	}else{
		base_sec_l.base_sec[0][smt_type].group_counter = 1;
		base_sec_l.base_sec[0][smt_type].group_idx = group_index;
		base_sec_l.base_sec[0][smt_type].len = len;
		base_sec_l.counter[smt_type]++;
	}
	base_sec_l.received++;
}

/*
pair data format:
group_num        + base_len ,  sample value:     83 = 1 + 82
CRC8（group_num）+ base_len,   sample value：    0x5e + 82
note:  group_num = group_index + 1
最多检查4组， 这样检查时每个数最多可能对应两个base_len， 减少计算
*/
atbm_uint8 smt_receive_magic(atbm_uint16 rx_mac_len, atbm_uint8 smt_type, atbm_int32 ch, atbm_int32 rssi)  //add1 or add3
{
	atbm_uint8 i = 0;
	atbm_uint8 try_base_len = 0;
	atbm_uint8 try_crc8 = 0;  

	atbm_uint8 try_group_num = 0;  //group_index + 1
//	wifi_printk(WIFI_ALWAYS, "smt_receive_magica %d ch:%d, rssi:%d\n",rx_mac_len,ch, rssi);

	if(rx_mac_len < MIN_MAC_BASE_LEN)
	{
		return 0;
	}

	//先保存一个配对值，save pair data, then compare;  不同的group间隔很大，不会串在一起，scan信道切换期间，只能检测到一组同步头
	if((rx_mac_len >= MAX_MIN_MAGIC_PAIR_LEN) && (rx_mac_len <= MAX_MAC_MAGIC_PAIR_LEN))
	{
		pre_pair_data[smt_type] = rx_mac_len;
	}

	if(pre_pair_data[smt_type] == 0)  //如果没有配对值，直接返回
	{
		return 0 ;
	}

#if 0
	for(i = 0; i< 8; i++)	//查找base len，通过配对信息。 这里组头和组号及CRC值都是固定的，通过查表，可以减少计算。
	{
		if(rx_mac_len > smt_mpdu_base_length[i] && (rx_mac_len <= smt_mpdu_base_length[i] + 4))
		{
			try_base_len = smt_mpdu_base_length[i];
			try_crc8  = (atbm_uint8)(pre_pair_data - try_base_len);
			try_group_num = (atbm_uint8)(rx_mac_len - try_base_len);
			wifi_printk(WIFI_ALWAYS, "smt_receive_magica %d try_crc8:%d, try_group_num:%d\n",rx_mac_len, try_crc8, try_group_num);
			if(smt_mpdu_pair_length[try_group_num] == try_crc8)
			{
				base_mac_len[smt_type] = try_base_len;
				cur_group_index = try_group_num - 1;
				if(cur_group_index < 4)
				{				
					//atbm_smt_state = ATBM_CONFIG_ST_GET_PAYLOAD;
					reset_group_data(cur_group_index);
					wifi_printk(WIFI_ALWAYS, "magic pair: cnt:%d base_len:%d, group:%d \n",  fromds_magic_counter, try_base_len, try_group_num);
					//if(smt_type == HT_ADDR1_TYPE)
					{
						fromds_magic_mask |= (1<<cur_group_index);
						fromds_magic_counter++;
						fromds_channel_rssi[ch] = rssi; 
						fromds_channel_paired_counter[ch]++;
						if(((fromds_magic_mask & 01) == 0x01)&&(fromds_magic_counter > 15))  //first pairs found.
						{
							fromds_channel = get_strong_channel();
							wifi_printk(WIFI_ALWAYS, "non ht magic done find pair: base_len:%d, g_num:%d ch:%d\n",  try_base_len, try_group_num, fromds_channel);		
							return 1;	//avoid 0
						}	
					}
					
				}
			}
		}			
	}
#endif
	for(i = 0; i< 8; i++)
	{
		if(rx_mac_len > smt_mpdu_base_length[i] && (rx_mac_len <= smt_mpdu_base_length[i] + 4)){
		{
			try_base_len = smt_mpdu_base_length[i];
			try_crc8  = (atbm_uint8)(pre_pair_data[smt_type] - try_base_len);
			try_group_num = (atbm_uint8)(rx_mac_len - try_base_len);
			if(smt_mpdu_pair_length[try_group_num] == try_crc8){
					set_base(smt_type, try_group_num, try_base_len);
					cur_group_index = try_group_num - 1;
					fromds_magic_mask |= (1<<cur_group_index);
					fromds_magic_counter++;
					fromds_channel_rssi[ch] = rssi; 
					fromds_channel_paired_counter[ch]++;
				}
			}
			if(base_sec_l.satisfied){
				reset_group_data(cur_group_index);
				fromds_channel = get_strong_channel();
				wifi_printk(WIFI_ALWAYS, "non ht magic done find pair: base_len:%d %d, g_num:%d ch:%d\n",  base_mac_len[0], base_mac_len[1], try_group_num, fromds_channel);		
				return 1;	//avoid 0
			}
		}
	}
	return 0;
}


/*
receive group index dynamically:
1, find and update group data index
2, reset group data,as data aways send after group index
*/
atbm_uint8 smt_receive_group_index(atbm_uint16 rx_mac_len, atbm_uint8 smt_type)
{
	atbm_uint8 try_crc8 = 0, crc8 = 0;
	static atbm_uint8 try_group_num = 0;  //group_num = group_index + 1
	static atbm_uint8 counter = 0;

	if(rx_mac_len < base_mac_len[smt_type])
	{
		return 0xff;
	}	

	if(rx_mac_len <= (base_mac_len[smt_type] + 0xff))  //group index CRC8 should not exceed 0xff
	{
		counter  = counter + 1;
	}else
	{
		return 0xff;
	}
	if(counter > 5)  //reset group_num after try each 5 packets, pair data not found in following 5 packets, may be fake value.
	{
		try_group_num = 0;
		counter = 0;
	}
	if((rx_mac_len > base_mac_len[smt_type]) && (rx_mac_len <=  (base_mac_len[smt_type] + 16)))  //maximum 16 groups
	{
		try_group_num = (atbm_uint8)(rx_mac_len - base_mac_len[smt_type]); //save pair data, then compare. 不同的group间隔很大，不会串在一起
		counter = 0;  //find key index value, reset counter to 0.
	}


	if(try_group_num == 0)  //group_num = group_index + 1, should not be 0
	{
		return 0xff ;
	}	
	//	printf("		try find pair\n");
	if((rx_mac_len >= base_mac_len[smt_type]) && (rx_mac_len <= (base_mac_len[smt_type] + 0xff)))  
	{			
		crc8  = smtcrc8(&try_group_num, 1);
		try_crc8 =  (atbm_uint8)(rx_mac_len - base_mac_len[smt_type]);

		if(crc8 == try_crc8)
		{			
			wifi_printk(WIFI_ALWAYS, "Paired g_num:%d g_data_cnt:%d\n", try_group_num, group_data_cur_counter);     //data aways follow group info.
			if(cur_group_index != (try_group_num - 1))  //debug new group.
			{			
				if(cur_group_index < 16)
				{

					wifi_printk(WIFI_ALWAYS, "update group pre merge!\n");
					fromap_sort_and_set_data();
					wifi_printk(WIFI_ALWAYS, "after meger!\n");
					fromap_merge_smt_data(cur_group_index, 1);  //low level data
				}
			}
			reset_group_data(cur_group_index);
			counter = 0;
			return  (try_group_num - 1);

		}
	}			

	return 0xff ;
}


/*收集当前组的数据*/
atbm_void smt_put_data_for_sort(atbm_uint8 index, atbm_uint8 len)
{
	atbm_uint8 u8iter = 0;
	//printf("index:%d\n", index);
	//try find the same length.
	int flag = 0;
	int min_counter_index;

	if(index >7)
		return;
	for(u8iter =0; u8iter < 4; u8iter++)
	{
		if(len == ht40_group_data[index].rdata[u8iter])
		{
			ht40_group_data[index].rdata_counter[u8iter] += 5;
			flag = 1;
		}else{
			ht40_group_data[index].rdata_counter[u8iter] = ht40_group_data[index].rdata_counter[u8iter] > 0 \
				?(ht40_group_data[index].rdata_counter[u8iter] - 1) : 0;
		}
	}

	if(flag){
		return;
	}

	for(u8iter =0; u8iter < 4; u8iter++)
	{
		if(ht40_group_data[index].rdata_counter[u8iter] == 0)
		{
			ht40_group_data[index].rdata[u8iter]= len;
			ht40_group_data[index].rdata_counter[u8iter] = 10;
			flag = 1;
			break;
		}
	}

	if(flag){
		return ;
	}

	wifi_printk(WIFI_ALWAYS, "data full, try replace index:%d len:%x\n", index, len);
	min_counter_index = 0;
	for(u8iter = 1; u8iter < 4; u8iter++){
		if(ht40_group_data[index].rdata_counter[min_counter_index] > ht40_group_data[index].rdata_counter[u8iter]){
			min_counter_index = u8iter;
		}
	}
	wifi_printk(WIFI_ALWAYS, "replaced pre min_counter_index : %d len:%x len:%x\n",min_counter_index, ht40_group_data[index].rdata[min_counter_index], len);
	ht40_group_data[index].rdata[min_counter_index]= len;  //replace current data.
	ht40_group_data[index].rdata_counter[min_counter_index] = 10;
//	printf("replaced pre len:%x len:%x\n",ht40_group_data[index].rdata[min_counter_index], len);
	return;
#if 0
	for(u8iter =0; u8iter < 4; u8iter++)
	{	 
		if((ht40_group_data[index].rdata_counter[u8iter] == 1)&&(u8iter == group_replace[index]))
		{
			ht40_group_data[index].rdata[u8iter]= len;  //replace current data.
			ht40_group_data[index].rdata_counter[u8iter] = 1;
			printf("replaced pre len:%x len:%x\n",ht40_group_data[index].rdata[u8iter], len);
			group_replace[index]++;   //next time will not replace this index data.
			if(group_replace[index] == 4)   
			{
				group_replace[index] = 0;
			}
			return;
		}
	}
#endif
}


atbm_void fromap_sort_and_set_data(atbm_void)
{
	atbm_uint8 u8idx = 0;
	atbm_uint16 u8iter = 0;
	atbm_uint8 tmp_data = 0;
	atbm_uint16 tmp_counter = 0;
	//printf("index:%d ",index);
#if 1
	for(u8idx = 0; u8idx< 8; u8idx++)
	{
		for(u8iter = 1; u8iter < 4; u8iter++)
		{
			if(ht40_group_data[u8idx].rdata_counter[0] < ht40_group_data[u8idx].rdata_counter[u8iter])  //swap for the max score data
			{
				tmp_data = ht40_group_data[u8idx].rdata[0];
				tmp_counter = ht40_group_data[u8idx].rdata_counter[0];
				ht40_group_data[u8idx].rdata[0] = ht40_group_data[u8idx].rdata[u8iter];
				ht40_group_data[u8idx].rdata_counter[0] = ht40_group_data[u8idx].rdata_counter[u8iter];
				ht40_group_data[u8idx].rdata[u8iter] = tmp_data;
				ht40_group_data[u8idx].rdata_counter[u8iter] = tmp_counter;
			}
		}
		if(ht40_group_data[u8idx].rdata_counter[0] == 0)  //no data any more.
		{
			wifi_printk(WIFI_ALWAYS, "Warn ht40_gdata u8idx:%d cnt 0,no data continue\n",u8idx);
			continue;
		}	       
		for(u8iter = 2; u8iter < 4; u8iter++)
		{
			if(ht40_group_data[u8idx].rdata_counter[1] < ht40_group_data[u8idx].rdata_counter[u8iter])	//swap for the second  score data
			{
				tmp_data = ht40_group_data[u8idx].rdata[1];
				tmp_counter = ht40_group_data[u8idx].rdata_counter[1];
				ht40_group_data[u8idx].rdata[1] =ht40_group_data[u8idx].rdata[u8iter];
				ht40_group_data[u8idx].rdata_counter[1] = ht40_group_data[u8idx].rdata_counter[u8iter];
				ht40_group_data[u8idx].rdata[u8iter] = tmp_data;
				ht40_group_data[u8idx].rdata_counter[u8iter] = tmp_counter;
			}
		}		
	}
#endif	
	//print data and counter.
	for(u8idx = 0; u8idx< 8; u8idx++)
	{
		wifi_printk(WIFI_ALWAYS, "idx:%d ",u8idx);
		for(u8iter =0; u8iter < 2; u8iter++)
		{
			wifi_printk(WIFI_ALWAYS, "%02x_%d ",ht40_group_data[u8idx].rdata[u8iter], 	ht40_group_data[u8idx].rdata_counter[u8iter]);		
		}
		wifi_printk(WIFI_ALWAYS, "\n");
	}

	for(u8idx = 0; u8idx< 8; u8idx++)
	{
		group_payload[u8idx] = ht40_group_data[u8idx].rdata[0];
	}
}

atbm_void fromap_merge_smt_data(atbm_uint8 cur_g_indx, atbm_uint8 score_level)
{
	atbm_uint8 i;
	atbm_uint8 start_idx, end_idx;
	atbm_uint8 update_flag = 0;
	atbm_uint8 tmp_crc[2];
	atbm_uint8 crc8 = 0;	
	atbm_uint8 iu8idx = 0;
	atbm_uint8 data_pos = 0;
	//printf("fromap_merge_smt_data\n");

	start_idx = cur_g_indx*8;
	end_idx =   start_idx + 8;

	if(score_level >1)
	{	
		for(iu8idx = 0; iu8idx< 8; iu8idx++)
		{		
			for(i =0; i < 2; i++)
			{
				if((group_payload[iu8idx] == ht40_group_data[iu8idx].rdata[i])&&(ht40_group_data[iu8idx].rdata_counter[i] > 0))
				{
					ht40_group_data[iu8idx].rdata_counter[i] = ht40_group_data[iu8idx].rdata_counter[i] + score_level; 					
				}				
			}		
		}
	}
	for(i = start_idx; i < end_idx; i++)
	{			
		if(ht40_group_data[i%8].rdata[0] == ht40_smt_legacy_data.p_legacy_data[i].rdata[2])
		{
			ht40_group_data[i%8].rdata_counter[0] = ht40_group_data[i%8].rdata_counter[0] + ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[2];

		}
		if(ht40_group_data[i%8].rdata[0] == ht40_smt_legacy_data.p_legacy_data[i].rdata[3])
		{
			ht40_group_data[i%8].rdata_counter[0] = ht40_group_data[i%8].rdata_counter[0] + ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[3];

		}
		if(ht40_group_data[i%8].rdata[1] == ht40_smt_legacy_data.p_legacy_data[i].rdata[2])
		{
			ht40_group_data[i%8].rdata_counter[1] = ht40_group_data[i%8].rdata_counter[1] + ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[2];
		}
		if(ht40_group_data[i%8].rdata[1] == ht40_smt_legacy_data.p_legacy_data[i].rdata[3])
		{
			ht40_group_data[i%8].rdata_counter[1] = ht40_group_data[i%8].rdata_counter[1] + ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[3];				
		}
	}

	for(i = start_idx; i < end_idx; i++)
	{
		if(ht40_smt_legacy_data.legacy_data_round[i] == 0)
		{
			atbm_memcpy(&ht40_smt_legacy_data.p_legacy_data[i], &ht40_group_data[i%8], 8);
			ht40_smt_legacy_data.legacy_data_round[i]++;
			wifi_printk(WIFI_ALWAYS, "merge copy for fist time i:%d\n", i);
		}else   //合并前两列数，最后重新排序，计算CRC
		{

			ht40_smt_legacy_data.p_legacy_data[i].rdata[2] = ht40_group_data[i%8].rdata[0];
			ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[2] = ht40_group_data[i%8].rdata_counter[0];

			ht40_smt_legacy_data.p_legacy_data[i].rdata[3] = ht40_group_data[i%8].rdata[1];
			ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[3] = ht40_group_data[i%8].rdata_counter[1];
			ht40_smt_legacy_data.legacy_data_round[i]++;
			update_flag = 1;
		}
	}	
	wifi_printk(WIFI_ALWAYS, "merge update_flag:%d! 0:only copy, 1:process\n", update_flag);	

	if(update_flag)
	{
		//merge 2,3 => 0,1
		for(i = start_idx; i < end_idx; i++)
		{			
			if(ht40_smt_legacy_data.p_legacy_data[i].rdata[0] == ht40_smt_legacy_data.p_legacy_data[i].rdata[2])
			{
				ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[0] = ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[0] + ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[2];
				ht40_smt_legacy_data.p_legacy_data[i].rdata[2]         = 0;
				ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[2] = 0;
			}
			if(ht40_smt_legacy_data.p_legacy_data[i].rdata[0] == ht40_smt_legacy_data.p_legacy_data[i].rdata[3])
			{
				ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[0] = ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[0] + ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[3];
				ht40_smt_legacy_data.p_legacy_data[i].rdata[3]         = 0;
				ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[3] = 0;
			}
			if(ht40_smt_legacy_data.p_legacy_data[i].rdata[1] == ht40_smt_legacy_data.p_legacy_data[i].rdata[2])
			{
				ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[1] = ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[1] + ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[2];
				ht40_smt_legacy_data.p_legacy_data[i].rdata[2]         = 0;
				ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[2] = 0;
			}
			if(ht40_smt_legacy_data.p_legacy_data[i].rdata[1] == ht40_smt_legacy_data.p_legacy_data[i].rdata[3])
			{
				ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[1] = ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[1] + ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[3];
				ht40_smt_legacy_data.p_legacy_data[i].rdata[3]         = 0;
				ht40_smt_legacy_data.p_legacy_data[i].rdata_counter[3] = 0;
			}
		}
		atbm_memcpy((atbm_uint8 *)&ht40_group_data[0],  (atbm_uint8 *)&ht40_smt_legacy_data.p_legacy_data[start_idx],  sizeof(struct ht40_smt_data)*8);
		//	atbm_memcpy(&ht40_group_data[0], &ht40_smt_legacy_data.p_legacy_data[start_idx], 8*8);
		//printf("merged data copy to current group for sort!\n");
		//printf(" \n");
		//debug
		//print data and counter.

		fromap_sort_and_set_data();
		//update legacy data
		atbm_memcpy(&ht40_smt_legacy_data.p_legacy_data[start_idx], &ht40_group_data[0], sizeof(struct ht40_smt_data)*8);


		wifi_printk(WIFI_ALWAYS, "merge data done check crc!\n");
		crc8 = smtcrc8(&group_payload[0], 7);  //most possible data is sorted data.
		tmp_crc[0] = crc8;
		tmp_crc[1] = cur_group_index;
		crc8 = smtcrc8(&tmp_crc[0], 2);

		//patch for index and index crc
		if((crc8&0x7f)!= group_payload[7]) 
		{
			for( iu8idx = 0; iu8idx< 8; iu8idx++)   //try second possible value, each time replace one value.
			{
				// iterate second possible value
				if( ht40_group_data[iu8idx].rdata_counter[1] > 0)
				{
					group_payload[iu8idx] = ht40_group_data[iu8idx].rdata[1];  //second possible value
				}
				crc8 = smtcrc8(&group_payload[0], 7);  //most possible data
				tmp_crc[0] = crc8;
				tmp_crc[1] = cur_group_index;
				crc8 = smtcrc8(&tmp_crc[0], 2);

				if((crc8&0x7f) == group_payload[7]) 
				{
					break;
				}
				group_payload[iu8idx] = ht40_group_data[iu8idx].rdata[0]; //change back data.
			}
		}
		if((crc8&0x7f) == group_payload[7])  //only check low 7 bit of CRC8 value
		{
			/*printf("group data done group index:%d group rcv counter:%d merged\n",cur_group_index, group_data_cur_counter);
			for( iu8idx = 0; iu8idx< 8; iu8idx++)
			{
				printf("%02x ",group_payload[iu8idx]);		
			}	
			*/		
			data_pos = cur_group_index * 8;
			atbm_memcpy(&smt_payload_data[data_pos], group_payload, 8);				
			group_received_mask |= (1<<cur_group_index);					
			if(cur_group_index == 0)
			{
				payload_total_length = smt_payload_data[0];   //get total length
				payload_total_xor    = smt_payload_data[1];   //get total data XOR
				payload_pwd_length   = smt_payload_data[2];   //get PWD length.
			}
			cur_group_index = 0xff;
		}
	}
}


static atbm_int32 atbm_smt_parse_payload(struct atbmwifi_vif *priv);
static atbm_int32 smt_fun_recv_payload(struct atbmwifi_vif *priv, atbm_uint8 smt_type, atbm_uint16 rxdata_raw)
{
	//atbm_uint8 crc_payload[2], crc_data = 0, u8iter =0;
	atbm_uint8 crc8 = 0;	
	atbm_uint16 data_len = 0;
	atbm_uint8 data_index = 0;
	atbm_uint8 data_pos = 0;
	atbm_uint8 iu8idx = 0;
	atbm_uint8 tmp_group_index = 0;
	atbm_uint8 tmp_crc[2];
	if(cur_group_index > 0x10)  //invalid group index, try sync group info.
	{
		tmp_group_index = smt_receive_group_index(rxdata_raw, smt_type);		
		if(tmp_group_index < 16) //force update current group index
		{
			cur_group_index = tmp_group_index;
		}
		return 0;
	}

	if((base_mac_len[smt_type] == 0) || (rxdata_raw < base_mac_len[smt_type])) //invalid packet length.
	{
		return 0;
	}

	tmp_group_index = smt_receive_group_index(rxdata_raw, smt_type);  //dynamically checking, may update group index
	if((tmp_group_index < 16)&&(tmp_group_index != cur_group_index))
	{
		cur_group_index = tmp_group_index;
		wifi_printk(WIFI_ALWAYS, "Update group index:%d  group_data_cur_counter:%d\n",cur_group_index, group_data_cur_counter);

		reset_group_data(cur_group_index);
	}

	if(group_mask & (1 << cur_group_index)){
		return 0;
	}

	data_len = rxdata_raw - base_mac_len[smt_type];
	data_index = (atbm_uint8)(data_len >>7);
	if(data_index > 7)  //each group have 8 data, max data index is 7
	{
		return 0;
	}

	group_data_cur_counter = group_data_cur_counter + 1;
	smt_put_data_for_sort(data_index, (data_len&0x7f));

	group_data_mask[cur_group_index]|= (1<<data_index);

	/*
	CRC checking takes time, must comply with following rules. 
	1, MASK of all 8 bits set
	2, Enough packets received. if net is clear, will receive most of the target packets, 
	if net is noisy, will receive many other packets, so set a thresh.
	*/
	if((group_data_mask[cur_group_index] == 0xff)&&(group_data_cur_counter > MIN_GROUP_DATA_COUNTER))
	{
//		wifi_printk(WIFI_ALWAYS, "##########line:%d########\n", __LINE__);
//		wsm_set_wol_enable(hw_priv, 0, 0);
		fromap_sort_and_set_data();
		crc8 = smtcrc8(&group_payload[0], 7);  //most possible data is sorted data.
		tmp_crc[0] = crc8;
		tmp_crc[1] = cur_group_index;
		crc8 = smtcrc8(&tmp_crc[0], 2);
		//patch for index and index crc
		if((crc8&0x7f)!= group_payload[7]) 
		{
			for( iu8idx = 0; iu8idx< 8; iu8idx++)   //try second possible value, each time replace one value.
			{
				// iterate second possible value
				if( ht40_group_data[iu8idx].rdata_counter[1] > 0)
				{
					group_payload[iu8idx] = ht40_group_data[iu8idx].rdata[1];  //second possible value
				}
				crc8 = smtcrc8(&group_payload[0], 7);  //most possible data				
				tmp_crc[0] = crc8;
				tmp_crc[1] = cur_group_index;
				crc8 = smtcrc8(&tmp_crc[0], 2);
				if((crc8&0x7f) == group_payload[7]) 
				{
					break;
				}
				group_payload[iu8idx] = ht40_group_data[iu8idx].rdata[0]; //change back data.
			}
		}
		if((crc8&0x7f) == group_payload[7])  //only check low 7 bit of CRC8 value
		{
			/*
			printf("group done index:%d group rcv cnt:%d\n",cur_group_index, group_data_cur_counter);
			for( iu8idx = 0; iu8idx< 8; iu8idx++)
			{
				printf("%02x ",group_payload[iu8idx]);		
			}
			printf("\n");
			*/
			data_pos = cur_group_index * 8;
			atbm_memcpy(&smt_payload_data[data_pos], group_payload, 8);				
			group_received_mask |= (1<<cur_group_index);					
			if(cur_group_index == 0)
			{
				payload_total_length = smt_payload_data[0];   //get total length
				payload_total_xor    = smt_payload_data[1];   //get total data XOR
				payload_pwd_length   = smt_payload_data[2];   //get PWD length.
			}

			fromap_merge_smt_data(cur_group_index, 2);  //实时数据接收正确后把数据放入历史数据区,  高信度数据			
			
			group_mask |= (1 << cur_group_index);
//			wifi_printk(WIFI_ALWAYS, "!!!!!!!!!group %d done group_mask:%x!!!!!!!!!!!!\n", cur_group_index, group_mask);
			cur_group_index = 0xff;
			reset_group_data(cur_group_index);
			if(check_smt_rx_done())
			{
				atbm_smt_parse_payload(priv);
			}
		}else
		{
			group_data_mask[cur_group_index] = 0;
			atbm_memset(&ht40_group_data[cur_group_index], 0, sizeof(struct ht40_smt_data));
			group_data_cur_counter = 0;
			wifi_printk(WIFI_ALWAYS, "group data CRC  error group index:%d\n",cur_group_index);
			cur_group_index = 0xff;
		}
	}
	return 0;
}

atbm_int32 check_smt_rx_done(void)
{
	atbm_int32 done_flag = 0;
	atbm_uint8 smt_len = 0;
	atbm_uint8 smt_group_num = 0;
	if(payload_total_length >= 5)
	{

		smt_len = 3 + ((payload_total_length - 3)*8 + 6)/7;    //total smartconfig data length exclude CRC.	
		smt_group_num = (smt_len + 6)/7;                     //there is 7 smartconfig data in each group and one byte CRC8 value.	
		if((group_received_mask & ((1<<smt_group_num) - 1)) == ((1<<smt_group_num) - 1))
		{
			wifi_printk(WIFI_ALWAYS, "smt payload done, check..!! mask:%d gnum:%d\n", group_received_mask, smt_group_num);
			done_flag = 1;
		}		
	}
	else
	{		
		done_flag = 0;
	}
	return done_flag;
}

static atbm_int32 atbm_smt_parse_payload(struct atbmwifi_vif *priv)
{	
	atbm_uint8 ssid_pwd_len  = 0; 
	atbm_uint8 i = 0, j = 0;
	atbm_uint16 valid_bit_len  =0;
	atbm_uint8  payload_byte_len = 0;
	//atbm_uint8  parse_byte_len  = 0;
	atbm_uint8  smt_counter = 0;
	atbm_uint8  total_xor = 0;
	
	atbm_uint8 *bitarrays = NULL;    //data convert used. 7bit payload data convert to 8bit smt data
	atbm_uint8 *bit2smt_data = NULL;
	atbm_uint8 *smt_bits_data = NULL;


	atbm_uint8 smt_len = 0, smt_group_num = 0;
	atbm_uint8  data_no_crc[128]= {0};
	//atbm_uint8  tmp_counter = 0;

	atbm_uint8 ssid_len =0, pwd_len = 0;
	ssid_pwd_len  = payload_total_length - 3;      //number of SSID and PWD bytes length.
	valid_bit_len    = ssid_pwd_len * 8;           //number of SSID and PWD bits length.
	payload_byte_len = (atbm_uint8)(valid_bit_len + 6)/7;      //MAC payload length. 8bits stream convert to 7bit stream
	//parse_byte_len   = (payload_byte_len * 8 + 6) /7;    //MAC payload length add CRC bytes

	bitarrays = (atbm_uint8*)atbm_kmalloc(512, GFP_KERNEL);
	bit2smt_data = (atbm_uint8*)atbm_kmalloc(128, GFP_KERNEL);
	smt_bits_data = (atbm_uint8*)atbm_kmalloc(128, GFP_KERNEL);

	if((bitarrays == NULL)||(bit2smt_data == NULL)||(smt_bits_data == NULL))
	{
		wifi_printk(WIFI_ALWAYS, "HT smt error malloc!\n");
	}

	/*remove CRC byte from each group: 8 bytes to 7 bytes  */
	smt_len = 3 + ((payload_total_length - 3)*8 + 6)/7; 
	smt_group_num = (smt_len + 6)/7; 
	for (i = 0; i < smt_group_num*8; i++)
	{
		data_no_crc[smt_counter] = smt_payload_data[i];
		if(((i+1) % 8) == 0)
		{
			continue;
		}
		smt_counter++;
	}

	for (i = 0; i < smt_group_num*8; i++)
	{
		smt_bits_data[i] =  data_no_crc[i+3];
	}
	/* generate bit stream from 7bit MAC payload */
	for (i = 0; i < payload_byte_len; i ++) {	
		for (j = 0; j < 7; j ++) {
			bitarrays[i * 7 + j] = (smt_bits_data[i] >> j) & 0x01;			
		}
	}
	/* generate 8bit PWD and SSID data from bit stream. */
	for(i = 0; i< ssid_pwd_len; i++)
	{
		for (j = 0, bit2smt_data[i] = 0; j < 8; j++) {
			bit2smt_data[i] |= bitarrays[i * 8 + j] << j;
		}
	}

	//PWD LEN归还XOR字节1bit数据， bit6位置存放最高XOR的bit
	if(smt_payload_data[2]&0x40)
	{	
		smt_payload_data[2]&=0x3f;
		smt_payload_data[1]|=0x80;
	}


	/*check XOR off all data
	1, get first data: total length
	2, XOR with itself and pwd_len
	3, XOR with all other bytes.
	*/	
	total_xor = smt_payload_data[0];  
	for(i = 1; i < 3; i++)
	{
		total_xor = total_xor ^ smt_payload_data[i];
	}	
	for(i = 0; i < ssid_pwd_len; i++)
	{
		total_xor = total_xor ^ bit2smt_data[i];
	}			
	atbm_kfree(bitarrays);
	atbm_kfree(smt_bits_data);
	bitarrays = NULL;
	smt_bits_data = NULL;

	if(total_xor == 0)
	{		
		//printf("VVVVV smt done XOR check passed!\n");
		
		pwd_len = smt_payload_data[2];
		ssid_len = ssid_pwd_len - pwd_len;

		if((pwd_len > 64)||(ssid_len > 32))
		{
			wifi_printk(WIFI_ALWAYS, "Error length, Smartconfig done XOR check passed!\n");
			atbm_kfree(bit2smt_data);
			bit2smt_data = NULL;
			return 0;
		}

		atbm_memcpy(smt_ssid, &bit2smt_data[0],ssid_len);
		atbm_memcpy(smt_pwd,  &bit2smt_data[ssid_len],pwd_len);
		//Command_SetPolling(1);		
		//printf(" \n");
		wifi_printk(WIFI_ALWAYS, "XOR passed! ssid :%s pwd:%s\n",smt_ssid,  smt_pwd);
		//printf(" \n");
		atbm_memset(priv->config.password,0,sizeof(priv->config.password));		
		atbm_memset(priv->config.ssid,0,sizeof(priv->config.ssid));


		priv->config.password_len = pwd_len;		
		atbm_memcpy(priv->config.password,smt_pwd, pwd_len);    		

		priv->config.ssid_len = ssid_len;
		atbm_memcpy(priv->config.ssid,smt_ssid, ssid_len);


		if(priv->config.password_len >= 8){
			priv->config.key_mgmt = ATBM_KEY_WPA2;
			priv->config.privacy =1;
		}
		else if(priv->config.password_len==5){
			priv->config.key_mgmt = ATBM_KEY_WEP;
			priv->config.privacy =1;
		}
		else if(priv->config.password_len==13){
			priv->config.key_mgmt = ATBM_KEY_WEP;
			priv->config.privacy =1;
		}
		else if(priv->config.password_len==0) {
			wifi_printk(WIFI_ALWAYS, "EY_NONE!!");   
			priv->config.key_mgmt = ATBM_KEY_NONE;
			priv->config.privacy =0;
		}
		else {
			wifi_printk(WIFI_ALWAYS, "<ERROR> smartconfig key len error %d\n",priv->config.password_len);
		}
		ht_state = HT_CONFIG_ST_GET_DONE;
			
		atbm_kfree(bit2smt_data);
		bit2smt_data = NULL;
		cur_group_index = 0xff;
		reset_group_data(cur_group_index);	
		atbmwifi_wpa_event_queue((atbm_void*)priv,ATBM_NULL,
			ATBM_NULL,WPA_EVENT__SMARTCONFIG_SUCCESS,ATBM_WPA_EVENT_NOACK);
		return 1;
	}
	atbm_kfree(bit2smt_data);
	bit2smt_data = NULL;
	wifi_printk(WIFI_ALWAYS, "Warning! smartconfig total_xor:%d\n", total_xor);
	return 0;
}

#define DEBUG_SIMU_CASE 1
atbm_void fromap_smartconfig_start(void) 
{	 
	malloc_legacy_data(); // will free late

	fromds_magic_mask = 0;
	fromds_magic_counter = 0;	
	fromds_channel = 15;
	group_received_mask = 0;

	cur_group_index = 0;
	payload_total_length = 0;
	payload_total_xor = 0;
	payload_pwd_length = 0;
	group_mask = 0;
	atbm_memset(&base_sec_l, 0, sizeof(base_sec_l));
	
	atbm_memset(fromds_channel_rssi, '\0', sizeof(fromds_channel_rssi));
		
	atbm_memset(fromds_channel_paired_counter, '\0', sizeof(fromds_channel_paired_counter));


	atbm_memset(pre_pair_data, 0, sizeof(pre_pair_data));	

	atbm_memset(smt_payload_data, '\0', sizeof(smt_payload_data));
	atbm_memset(ht40_group_data, '\0', sizeof(ht40_group_data));
//	atbm_memset(group_replace, '\0', sizeof(group_replace));

	base_mac_len[ADDR1_TYPE] = 0;
	base_mac_len[ADDR3_TYPE] = 0;
	smartconfig_clear_magic_channel();
	ht_state = HT_CONFIG_ST_GET_MAGIC;	
	//atbm_smt_state = ATBM_CONFIG_ST_GET_MAGIC
}

atbm_uint16 packets[4] = {172, 79, 174, 81};
atbm_uint16 first[4] = { 0 };

const atbm_uint8 atbm_smart_config_magic_mac[3][6] ={
		{0x01,0x00,0x5e,0x7f,0xff,0x01},//magic
		{0x01,0x00,0x5e,0x7f,0xff,0x02},//checksum
		{0x01,0x00,0x5e,0x7f,0xff,0x03},//payload
};

atbm_int32 atbm_smt_ht_rx_handler(struct atbmwifi_vif *priv,char * data,int rx_mac_len,int channel, int rssi)
{
	atbm_uint8 smt_magic_rev_type = 0;
	atbm_uint8 smt_type;
	
	if(rx_mac_len < HT_PAYLOAD_EXTR_LENGTH + 60)
		return 0;
	if(rx_mac_len > HT_PAYLOAD_MAX_LENGTH)
		return 0;

	if(!memcmp(atbm_smart_config_magic_mac[0],&data[4],5)){
		smt_type = ADDR1_TYPE;
	}else if(!memcmp(atbm_smart_config_magic_mac[0],&data[4+12],5)){
		smt_type = ADDR3_TYPE;
	}else{
		return 0;
	}

	rx_mac_len = rx_mac_len - HT_PAYLOAD_EXTR_LENGTH;  //all packet contains this extra length.

	if(ht_state  == (int)(HT_CONFIG_ST_GET_MAGIC)){	
		smt_magic_rev_type = smt_receive_magic(rx_mac_len, smt_type, channel, rssi);
		if(smt_magic_rev_type){
			if(priv->st_status < CONFIG_ST_SWITCH_PAYLOAD_CH){
				priv->st_status = CONFIG_ST_SWITCH_PAYLOAD_CH;//compatible with atbm smartconfig

				//patch: if 11b signal, try used the strong signal channel.
				if(fromds_channel < 14)
				{
					channel = fromds_channel;
				}			
				priv->st_configchannel = channel;
				wifi_printk(WIFI_ALWAYS, "will fixch: g_hw_prv.st_status:%d, channel:%d \n", priv->st_status, channel);
				atbm_SleepMs(50);
				if(priv->st_status < CONFIG_ST_SWITCH_PAYLOAD_CH)
				{
					wifi_printk(WIFI_ALWAYS, "will fixch: g_hw_prv.st_status:%d, channel:%d \n", priv->st_status, channel);
					priv->st_status = CONFIG_ST_SWITCH_PAYLOAD_CH;//compatible with atbm smartconfig
					priv->st_configchannel = channel;
				}
			}
			//ew_rx_last_time= hal_get_os_second_time();
			if(priv->st_status == CONFIG_ST_GET_PAYLOAD)
			{
				ht_state = ATBM_CONFIG_ST_GET_PAYLOAD;	//compatible with ew
				}	
			}
			//Smart_config_rssi_thresh(ht_rssi[smt_type] - 25);  //-28	-49 test shows
		
	}
	else if(ht_state == (int)(ATBM_CONFIG_ST_GET_PAYLOAD)){		
		smt_fun_recv_payload(priv, smt_type, rx_mac_len);
	}	
	return 0;
}

/*********************************************
End of new smartconfig RX code
*******************************************/
atbm_void atbm_ht_smt_setting(atbm_void)
{	
	fromap_smartconfig_start();
}

atbm_int32 smartconfig_start_rx(struct atbmwifi_vif *priv,struct atbm_buff *skb,int channel )
{
//	wifi_printk(WIFI_ALWAYS, "smartconfig_rx1 len %d,st %d\n",ATBM_OS_SKB_LEN(skb),hw_priv->st_status);
#if 1
	atbm_int32 rssi = 0;
	struct atbmwifi_ieee80211_rx_status *hw_hdr = ATBM_IEEE80211_SKB_RXCB(skb);
	rssi = hw_hdr->signal;	
    //printf("rssi:%d\n", rssi);
	atbm_smt_ht_rx_handler(priv,(char *) ATBM_OS_SKB_DATA(skb), ATBM_OS_SKB_LEN(skb),channel, rssi);

#else 
	if(hw_priv->st_status == CONFIG_ST_GET_MAGIC){
			smartconfig_step_1(hw_priv,(char *)ATBM_OS_SKB_DATA(skb),ATBM_OS_SKB_LEN(skb),channel);
	}
	else if(hw_priv->st_status == CONFIG_ST_GET_PAYLOAD){
			smartconfig_step_2(hw_priv,(char *)ATBM_OS_SKB_DATA(skb),ATBM_OS_SKB_LEN(skb));
        //ew_smartconfig_step_save_fifo(skb, channel);
	}
#endif
	return 0;
}

 atbm_void atbm_wifi_smart_config_rx(struct atbmwifi_common *hw_priv,struct atbm_buff *skb,int channel )
{		
	//smartconfig_start_rx(hw_priv,skb,channel);
	//TODO: add custom data hander
	//custom_datahandler(ATBM_OS_SKB_DATA(skb), ATBM_OS_SKB_LEN(skb));
	wifi_printk(WIFI_DBG_ERROR,"atbm_wifi_smart_config_rx: len %d, ch %d\n", ATBM_OS_SKB_LEN(skb), channel);	
	return;
}


 atbm_uint8 atbmwifi_monitor_enable(atbm_uint8 if_id)
{
	struct atbmwifi_vif *priv;
	priv= _atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,if_id);
	if(!atbm_wifi_initialed(priv->if_id)){
		wifi_printk(WIFI_DBG_ERROR,"atbm_wifi_initialed err\n");	
		return -1;
	}
	AT_WDisConnect_vif(priv,ATBM_NULL);
	atbm_mdelay(10);
	//atbmwifi_monitor_channel(1);
	return 0;
}

 atbm_void atbmwifi_monitor_disable(atbm_uint8 if_id)
{
	struct atbmwifi_vif *priv;
	struct wsm_reset reset;
	priv= _atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,if_id);
	reset.reset_statistics = ATBM_TRUE;
	/*start smartconfig*/
	priv->scan.scan_smartconfig = 0;
	priv->st_status = CONFIG_ST_IDLE;
	wsm_reset(&g_hw_prv,&reset,0);
}

#define WSM_SET_CHANTYPE_STARTCONFIG		BIT(4)
 atbm_int32 atbmwifi_monitor_channel(atbm_int32 channelnum,atbm_uint8 if_id)
{
	struct atbmwifi_vif *priv;
	priv= _atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,if_id);
			
	if((channelnum < 1) || (channelnum > 13 )){
		wifi_printk(WIFI_ALWAYS, "invalid channel number,range [1~13]\n");
		return -1;
	}
	//
	priv->scan_expire = 2;
	priv->scan_no_connect_back = 1;
	priv->scan_no_connect = 1;
	/*start smartconfig*/
	priv->scan.scan_smartconfig = 1;
	priv->st_status = CONFIG_ST_START;
	
	{
		struct wsm_set_chantype arg = {
			.band = 0,			//0:2.4G,1:5G
			.flag = 0,			//no use
			.channelNumber = channelnum,	// channel number
			.channelType = CH_OFF_20,	// channel type
		};

		arg.flag |= WSM_SET_CHANTYPE_STARTCONFIG;
		
		if(channelnum <= 11){
			arg.channelType = ATBM_NL80211_CHAN_HT40PLUS;//20M
		}
		else {
			arg.channelType = ATBM_NL80211_CHAN_HT40MINUS;//20M
		}
		wsm_set_chantype_func(&g_hw_prv,&arg,0);
	}
	return 0;
}

atbm_void smartconfig_success_notify(struct atbmwifi_vif *priv)
{
	atbmwifi_stop_smartconfig(priv);
	priv->st_status = CONFIG_ST_DONE_SUCCESS;

}


