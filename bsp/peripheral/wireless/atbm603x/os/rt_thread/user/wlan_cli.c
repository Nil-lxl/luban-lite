#include <stdlib.h>
#include <string.h>
#include "atbm_hal.h"
#include "atbm_wifi_driver_api.h"
#include "atbm_wsm.h"
#include "strings.h"
#include "atbm_etf.h"

typedef void(*T_pfCMD)(int argc, char **args);
typedef struct _wifi_cmd_entry
{
    char cmd_name[32];
    T_pfCMD cmd_entry;
}wifi_cmd_entry_t;

extern struct atbmwifi_common g_hw_prv;
extern char ConvertHexChar(char ch);
int atbm_str2mac(char *macstr, unsigned char *buf)
{
	char tmp = 0;
	for(int i =0; i< 5; i++)
	{
		tmp = ConvertHexChar(*macstr++);
		if(tmp < 0){
			return -1;
		}
		buf[i] = tmp*16;
		tmp = ConvertHexChar(*macstr++);
		if(tmp < 0){
			return -1;
		}
		buf[i] = buf[i]+tmp;
		macstr++;
	}
	tmp= ConvertHexChar(*macstr++);
	if(tmp < 0){
		return -1;
	}
	buf[5] = tmp*16;
	tmp = ConvertHexChar(*macstr);
	if(tmp < 0){
		return -1;
	}
	buf[5] = buf[5]+tmp;
	return 0;
}
static void UartCmd_StartTx(int argc, char **args){
	atbm_uint32 uiChannel, uiRate, is_40M, uiGreenfield;
	atbm_int32 flag = 0, i = 0, j = 0;
	char uiRatePtr[4] = {0};
	if(argc < 6){
		printf("need more parameter,please try again!\n");
		return;
	}
	uiChannel = atoi(args[2]);

	while(args[3][j] != '\0'){
		if(args[3][j] == '.'){
			flag = 1;
		}else{
			uiRatePtr[i++] = args[3][j];
		}
		j++;
	}

	uiRate = flag? atoi(uiRatePtr) : atoi(uiRatePtr) * 10;
	is_40M = atoi(args[4]);
	uiGreenfield = atoi(args[5]);
	printf("Ch : %d, Rate : %d, is_40M : %d greenfiled: %d\n", uiChannel, uiRate, is_40M, uiGreenfield);
	if(atbm_etf_start_tx(uiChannel, uiRate, is_40M/*40M*/, uiGreenfield/*greedfiled*/) < 0){
	    printf("start tx failed!\n");
	}
}

static void UartCmd_StopTx(int argc, char **args){
	atbm_etf_stop_tx();
}

static void UartCmd_StartRx(int argc, char **args){
	atbm_uint32 uiChannel, is_40M;
	if(argc < 4){
	    printf("need more parameter,please try again!\n");
		return;
	}
	uiChannel = atoi(args[2]);
	is_40M = atoi(args[3]);
	printf("Ch : %d is_40M: %d\n", uiChannel, is_40M);
	if(atbm_etf_start_rx(uiChannel, is_40M/*40M*/) < 0){
	    printf("start etf rx failed!\n");
		return;
	}
}

static void UartCmd_StopRx(int argc, char **args)
{
	atbm_etf_stop_rx();
}
extern atbm_int32 atbmwifi_enable_lmaclog(atbm_uint32 value );
static void atbm_enable_logprint(int argc, char **args)
{
	int enable;
	if(argc < 3){
	    printf("need more params!!\n");
		return;
	}
	enable = atoi(args[2]);
	atbmwifi_enable_lmaclog(enable);
}

static void atbm_wifi_fwcmd(int argc, char **args)
{
	char cmd[64]={0};
	int cmd_len=0;
	if(argc < 3){
		printf("need more args\n");
		return;
	}
	cmd_len=strlen(args[2]);
	if(cmd_len >sizeof(cmd)){
		printf("cmd too long \n");
		return ;
	}
	strcpy(cmd,args[2]);
	cmd[cmd_len]=' ';
	atbmwifi_enable_lmaclog(1);
	wsm_write_mib(&g_hw_prv, WSM_MIB_ID_FW_CMD,cmd, cmd_len+1, 0);
}

#if CONFIG_WPS
static void UartCmd_WPS_Control(int argc, char **args){
    struct atbmwifi_vif *priv = _atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,0);
	if(argc < 3){
		wifi_printk(WIFI_ALWAYS,"need more param!\n");
	}
	if(!strcmp("PBC", args[2]) || !strcmp("pbc", args[2])){
		iot_printf("WPS:start PBC\n");
		atbmwps_start_pbc(priv, NULL);
	}else if((!strcmp("PIN", args[2]) || !strcmp("pin", args[2])) && argc >= 4){
		iot_printf("WPS:start PIN %s\n", args[3]);
		atbmwps_start_pin(priv, args[3], NULL, 0);
	}else if(!strcmp("STOP", args[2]) || !strcmp("stop", args[2])){
		iot_printf("WPS: stop\n");
		atbmwps_cancel(priv);
	}
}
#endif

static void UartCmd_MFGCarrierTone(int argc, char **args)
{
    atbm_uint32 uiChannel;
    atbm_int32  iError;

    uiChannel = atoi(args[2]);
    printf("Ch : %d \n", uiChannel);
    iError = atbm_wifi_mfg_CarrierTone(uiChannel);
    printf("CarrierTone Status : %d \n", iError);
}

extern int hex2byte(const char *hex);
extern int DCXOCodeWrite(struct atbmwifi_common *hw_priv,atbm_uint8 data);
extern int atbm_save_efuse(struct atbmwifi_common *hw_priv, struct efuse_headr *efuse_save);
static void atbm_wifi_efuse_set(int argc, char **args)
{
	int i;
	int iError = -1;
	atbm_uint32 writeEfuseFlag = 0;//0:write to register;1:write to efuse
	ATBM_BOOL bDcxo = 0, bGain = 0;
	atbm_int8 cmdBuf[128] = {0};
	struct efuse_headr efuse_data_local;
	struct atbmwifi_vif *vif=_atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,0);
	struct atbmwifi_common *hw_priv = vif->hw_priv;
	memset(&efuse_data_local, 0, sizeof(struct efuse_headr));
	
	memcpy(&efuse_data_local, &hw_priv->efuse, sizeof(struct efuse_headr));
	if(argc < 4){
		printf("need more params!!\n");
		return;
	}
	if(memcmp(args[2], "setEfuse_dcxo", 13) == 0)
	{
		if(argc < 4){
			printf("setEfuse_deltagain need more params!!\n");
			return;
		}
		bDcxo = 1;
		efuse_data_local.dcxo_trim = atoi(args[3]);
		if(argc == 5)
			writeEfuseFlag = atoi(args[4]);
		printf("set efuse data is dcxo[%d]\n",efuse_data_local.dcxo_trim);
	}
	else if(memcmp(args[2], "setEfuse_deltagain", 18) == 0)
	{
		if(argc < 6){
			printf("setEfuse_deltagain need more params!!\n");
			return;
		}
		bGain = 1;
		efuse_data_local.delta_gain1 = atoi(args[3]);
		efuse_data_local.delta_gain2 = atoi(args[4]);
		efuse_data_local.delta_gain3 = atoi(args[5]);
		if(argc == 7)
			writeEfuseFlag = atoi(args[6]);
		printf("set efuse data is delta_gain[%d,%d,%d]\n",
			efuse_data_local.delta_gain1,efuse_data_local.delta_gain2,efuse_data_local.delta_gain3);
	}
	else if(memcmp(args[2], "setEfuse_mac", 12) == 0)
	{
		if(argc < 4){
			printf("setEfuse_mac need more params!!\n");
			return;
		}
		atbm_str2mac(args[3],efuse_data_local.mac);
		writeEfuseFlag = 1;
		printf("set efuse data is mac["MACSTR"]\n",MAC2STR(efuse_data_local.mac));

	}
	if(writeEfuseFlag == 1)
	{
		iError = atbm_save_efuse(hw_priv, &efuse_data_local);
		if(iError){
			printf("set efuse data failed:%d!!!\n", iError);
		}
		else
		{
			memcpy(&hw_priv->efuse, &efuse_data_local, sizeof(efuse_data_local));
			printf("set efuse data success\n");
		}
	}
	else
	{
		if(bDcxo)
		{
			iError = DCXOCodeWrite(hw_priv, efuse_data_local.dcxo_trim);
			if(iError)
				printf("DCXOCodeWrite failed:%d!!!\n", iError);
			else
				printf("DCXOCodeWrite success\n");
		}
		if(bGain)
		{
			memset(cmdBuf, 0, sizeof(cmdBuf));
			sprintf(cmdBuf, "set_txpwr_and_dcxo,%d,%d,%d,%d ", efuse_data_local.delta_gain1,
				efuse_data_local.delta_gain2, efuse_data_local.delta_gain3, efuse_data_local.dcxo_trim);
			
			printf("cmd: %s\n", cmdBuf);
			iError = wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD, cmdBuf, strlen(cmdBuf), 0);
			if(iError){
				printf("%s: write mib failed(%d). \n",__func__, iError);
			}
			else
				printf("deltagain write success\n");
		}
	}
	
}

static void atbm_wifi_set_all_efuse(int argc, char **args)
{
	int i = 0;
	int iError = -1;
	struct atbmwifi_vif *vif=_atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,0);
	struct atbmwifi_common *hw_priv = vif->hw_priv;
	int efuse_remainbit = 0;
	struct efuse_headr tmpEfuse;

	memset(&tmpEfuse, 0, sizeof(tmpEfuse));
	memcpy(&tmpEfuse, &hw_priv->efuse, sizeof(struct efuse_headr));	

	if(argc < 7)
	{
		printf("need more params\n");
		return ;
	}
	
	tmpEfuse.dcxo_trim = atoi(args[2]);
	if((tmpEfuse.dcxo_trim < 0) || (tmpEfuse.dcxo_trim > 127))
	{
		printf("invalid dcxo:%d\n", tmpEfuse.dcxo_trim);
		return ;
	}
	
	tmpEfuse.delta_gain1 = atoi(args[3]);
	if((tmpEfuse.delta_gain1 < 0) || (tmpEfuse.delta_gain1 > 31))
	{
		printf("invalid delta_gain1:%d\n", tmpEfuse.delta_gain1);
		return ;
	}
	tmpEfuse.delta_gain2 = atoi(args[4]);
	if((tmpEfuse.delta_gain2 < 0) || (tmpEfuse.delta_gain2 > 31))
	{
		printf("invalid delta_gain2:%d\n", tmpEfuse.delta_gain2);
		return ;
	}
	tmpEfuse.delta_gain3 = atoi(args[5]);
	if((tmpEfuse.delta_gain3 < 0) || (tmpEfuse.delta_gain3 > 31))
	{
		printf("invalid delta_gain3:%d\n", tmpEfuse.delta_gain3);
		return ;
	}
	atbm_str2mac(args[6],tmpEfuse.mac);
	printf("efuse data:[%d,%d,%d,%d,%d,%d,%d,%d,"MACSTR"]\n",
		tmpEfuse.version,tmpEfuse.dcxo_trim,
		tmpEfuse.delta_gain1,tmpEfuse.delta_gain2,tmpEfuse.delta_gain3,
		tmpEfuse.Tj_room,tmpEfuse.PowerSupplySel,tmpEfuse.topref_ctrl_bias_res_trim,
		MAC2STR(tmpEfuse.mac));

	iError = atbm_save_efuse(hw_priv, &tmpEfuse);
	if (iError == 0)
	{
		printf("setEfuse success \n");
		memcpy(&hw_priv->efuse, &tmpEfuse, sizeof(tmpEfuse));
	}else
	{
		printf("setEfuse failed [%d]\n", iError);
	}
	iError = wsm_get_efuse_remain_bit(hw_priv, &efuse_remainbit, sizeof(efuse_remainbit));
	printf("##after write efuse_remainbit:%d##\n", efuse_remainbit);

}

static void atbm_wifi_efuse_get(int argc, char **args)
{
	int iError;
	struct efuse_headr efuse_data_local;
	struct atbmwifi_vif *vif=_atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,0);
	struct atbmwifi_common *hw_priv = vif->hw_priv;
	memset(&efuse_data_local, 0, sizeof(struct efuse_headr));
	iError = wsm_get_efuse_data(hw_priv, &efuse_data_local, sizeof(struct efuse_headr));
	if(iError){
		printf("get efuse data error!!\n");
	}else{
		printf("Get efuse data is [%d,%d,%d,%d,%d,%d,%d,%d,%02x:%02x:%02x:%02x:%02x:%02x]\n",
				efuse_data_local.version,efuse_data_local.dcxo_trim,efuse_data_local.delta_gain1,efuse_data_local.delta_gain2,efuse_data_local.delta_gain3,
				efuse_data_local.Tj_room,efuse_data_local.topref_ctrl_bias_res_trim,efuse_data_local.PowerSupplySel,efuse_data_local.mac[0],efuse_data_local.mac[1],
				efuse_data_local.mac[2],efuse_data_local.mac[3],efuse_data_local.mac[4],efuse_data_local.mac[5]);
	}
}
//get first efuse data
static void atbm_wifi_efuse_getfirst(int argc, char **args)
{
	int iError;
	struct efuse_headr tmpEfuse;
	struct atbmwifi_vif *vif=_atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,0);
	struct atbmwifi_common *hw_priv = vif->hw_priv;
	memset(&tmpEfuse, 0, sizeof(struct efuse_headr));
	iError = wsm_get_efuse_first_data(hw_priv, &tmpEfuse, sizeof(struct efuse_headr));
	if(iError){
		printf("get efuse first data error!!\n");
	}else{
		printf("efuse first data is [%d,%d,%d,%d,%d,%d,%d,%d,%02x:%02x:%02x:%02x:%02x:%02x]\n",
				tmpEfuse.version,tmpEfuse.dcxo_trim,tmpEfuse.delta_gain1,tmpEfuse.delta_gain2,tmpEfuse.delta_gain3,
				tmpEfuse.Tj_room,tmpEfuse.topref_ctrl_bias_res_trim,tmpEfuse.PowerSupplySel,tmpEfuse.mac[0],tmpEfuse.mac[1],
				tmpEfuse.mac[2],tmpEfuse.mac[3],tmpEfuse.mac[4],tmpEfuse.mac[5]);
	}
}
//get all efuse data for debug
static void atbm_wifi_get_efuse_all_data(int argc, char **args)
{
	int iError,i = 0;
	struct atbmwifi_vif *vif=_atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,0);
	struct atbmwifi_common *hw_priv = vif->hw_priv;
	unsigned char buffer[128] = {0};

	memset(buffer,0, sizeof(buffer));
	if ((iError = wsm_get_efuse_all_data(hw_priv, (void *)&buffer, sizeof(buffer))) == 0){	
		printf("all efuse data:");
		for(i=0;i<109;i++)
			printf("%02x ",buffer[i]);
		printf("\n");
	}
	else{
		printf("read all efuse failed\n");
	}
}

//get efuse remain sapce
static void atbm_wifi_get_efuse_free_space(int argc, char **args)
{
	int iError;
	struct atbmwifi_vif *vif=_atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,0);
	struct atbmwifi_common *hw_priv = vif->hw_priv;
	int efuse_remainbit = 0;
	iError = wsm_get_efuse_remain_bit(hw_priv, &efuse_remainbit, sizeof(int));
	
	if(iError){
		printf("get efuse first data error!!\n");
	}else{
		printf("efuse free space:[%d] bit \n", efuse_remainbit);
	}

}
//get first mac
static void atbm_wifi_get_First_mac(int argc, char **args)
{
	//int index;
    unsigned char macAddr[6] = {0};
	int iError;
	struct atbmwifi_vif *vif=_atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,0);
	struct atbmwifi_common *hw_priv = vif->hw_priv;

	if ((iError = wsm_get_efuse_first_mac(hw_priv, &macAddr[0])) == 0){
		printf("firstMacAddr:[%02x:%02x:%02x:%02x:%02x:%02x]\n", macAddr[0], macAddr[1], macAddr[2],
			macAddr[3], macAddr[4], macAddr[5]);
	}
	else{
		printf("read mac address failed\n");
	}

}

//set power calibration flag
static void atbm_wifi_set_calibrate_flag(int argc, char **args)
{
	int i = 0;
	int iError = -1;
	struct atbmwifi_vif *vif=_atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,0);
	struct atbmwifi_common *hw_priv = vif->hw_priv;
	int caliFlag;
	char writebuf[128] = "";

	caliFlag = atoi(args[2]);

	if((caliFlag != 0) && (caliFlag != 1))
	{
		printf("Invalid parameter:%s\n", caliFlag);
		return ;
	}

	if(caliFlag == 1)
	{
		memset(writebuf, 0, sizeof(writebuf));
		sprintf(writebuf, "set_cali_flag,%d ", caliFlag);		
		printf("cmd: %s\n", writebuf);
	}
	else
	{
		memset(writebuf, 0, sizeof(writebuf));
		sprintf(writebuf, "set_cali_flag,%d ", caliFlag);			
		printf("cmd: %s\n", writebuf);
	}
	iError = wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD, writebuf, strlen(writebuf), 0);
	if(iError < 0){
		printf("%s: write mib failed(%d). \n",__func__, iError);
	}
}

//Set the transmit power at different mode
static void atbm_wifi_set_mode_power(int argc, char **args)
{
	int iError = -1, i = 0;
	atbm_int8 cmdBuf[128] = "";
	struct atbmwifi_vif *vif=_atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,0);
	struct atbmwifi_common *hw_priv = vif->hw_priv;
	atbm_int32 if_id = -1;
	static atbm_int8 deltagain[4]={0};
	static atbm_int8 bgain[3]={0};
	static atbm_int8 gngain[3]={0};

	
	if(memcmp(args[2], "gengain", 7) == 0)
	{
		if(argc < 7)
		{
			printf("need more params!!\n");
			return ;
		}
		for(i=0;i<4;i++)
		{
			deltagain[i] = atoi(args[i+3]);
			printf("deltagain[%d]:%d\n", i, deltagain[i]);
		}
	}
	else if(memcmp(args[2], "bgain", 5) == 0)
	{
		if(argc < 6)
		{
			printf("need more params!!\n");
			return ;
		}
		for(i=0;i<3;i++)
		{
			bgain[i] = atoi(args[i+3]);
			printf("bgain[%d]:%d\n", i, bgain[i]);
		}
	}
	else if(memcmp(args[2], "gngain", 6) == 0)
	{
		if(argc < 6)
		{
			printf("need more params!!\n");
			return ;
		}
		for(i=0;i<3;i++)
		{
			gngain[i] = atoi(args[i+3]);
			printf("gngain[%d]:%d\n", i, gngain[i]);
		}
	}
	else if(memcmp(args[2], "apply", 5) == 0)
	{
		memset(cmdBuf, 0, sizeof(cmdBuf));
		sprintf(cmdBuf, "set_txpwr_and_dcxo,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d ",deltagain[0], deltagain[1], deltagain[2], deltagain[3],
			bgain[0], bgain[1], bgain[2],gngain[0], gngain[1], gngain[2]);
		if(deltagain[3]==0)
			deltagain[3] = hw_priv->efuse.dcxo_trim;			
		printf("cmd: %s\n", cmdBuf);
		iError = wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD, cmdBuf, strlen(cmdBuf), if_id);
		if(iError != 0){
			printf("write mib failed(%d). \n", iError);
		}
		else
			printf("set mode power success\n");
		char strbuf[256]={0};
		sprintf(strbuf,"delta_gain1:%d delta_gain2:%d delta_gain3:%d dcxo:%d\n"
			"b_delta_gain1:%d b_delta_gain2:%d b_delta_gain3:%d\n"
			"gn_delta_gain1:%d gn_delta_gain2:%d gn_delta_gain3:%d\n",
			deltagain[0], deltagain[1], deltagain[2], deltagain[3],
			bgain[0], bgain[1], bgain[2],gngain[0], gngain[1], gngain[2]);
		//access_file(WIFI_TXPWER_PATH,strbuf,strlen(strbuf),0);
	}
		
}
//Set the transmit power at different rates
static void atbm_wifi_set_rate_power(int argc, char **args)
{
	int iError = 0, i = 0;
	struct atbmwifi_vif *vif=_atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,0);
	struct atbmwifi_common *hw_priv = vif->hw_priv;
	atbm_uint8 index = 0;
	static atbm_int8 rate_txpower[23] = {0};//validfalg,data
	
	if(memcmp(args[2], "power", 5) == 0)
	{
		if(argc < 5)
		{
			printf("need more params!!\n");
			return ;
		}
		index = atoi(args[3]);
		if((0 <= index) && (index < 23))
		{
			rate_txpower[index] = atoi(args[4]);
			printf("rate_txpower[%d]:%d\n", index, rate_txpower[index]);
		}
		else
			printf("Invalid index:[%d]\n", index);
	}
	else if(memcmp(args[2], "apply", 5) == 0)
	{
		for(i=22;i>11;i--)
			rate_txpower[i] = rate_txpower[i-1];
		rate_txpower[11] = 1;
		{
			printf("hw_priv->wsm_caps.firmwareVersion:%d\n",hw_priv->wsm_caps.firmwareVersion);
			if(hw_priv->wsm_caps.firmwareVersion > 12040)
				iError = wsm_write_mib(hw_priv, WSM_MIB_ID_SET_RATE_TX_POWER, rate_txpower, sizeof(rate_txpower), 0);
			else
				iError = wsm_write_mib(hw_priv, WSM_MIB_ID_SET_RATE_TX_POWER, rate_txpower, 12, 0);
			if(iError < 0){
				printf("write mib failed(%d). \n", iError);
			}
			else
				printf("set rate power success\n");
			char strbuf[256]={0};
			sprintf(strbuf,"b_1M_2M=%d\nb_5_5M_11M=%d\ng_6M_n_6_5M=%d\ng_9M=%d\ng_12M_n_13M=%d\n"
					"g_18M_n_19_5M=%d\ng_24M_n_26M=%d\ng_36M_n_39M=%d\ng_48M_n_52M=%d\ng_54M_n_58_5M=%d\nn_65M=%d\n",
					rate_txpower[0],rate_txpower[1],rate_txpower[2],rate_txpower[3],rate_txpower[4],rate_txpower[5],
					rate_txpower[6],rate_txpower[7],rate_txpower[8],rate_txpower[9],rate_txpower[10]);
			//access_file(WIFI_RATE_PWER_PATH,strbuf,strlen(strbuf),0);
		}
	}
}

/* wifi test command entry table, we can add new cmd hear */
static wifi_cmd_entry_t s_test_cmd_table[] = 
{
/* WIFI test cmd start*/
	{"start_tx",       UartCmd_StartTx},
	{"stop_tx",       UartCmd_StopTx},
	{"start_rx",       UartCmd_StartRx},
	{"stop_rx",       UartCmd_StopRx},
	{"mfgcarriertone",       UartCmd_MFGCarrierTone},
#if CONFIG_WPS
	{"wps_mode", UartCmd_WPS_Control},
#endif
    {"lmac_log",       atbm_enable_logprint},
    {"set_cali_flag",      atbm_wifi_set_calibrate_flag},
    {"set_mode_power",      atbm_wifi_set_mode_power},
    {"set_rate_power",      atbm_wifi_set_rate_power},
    {"efuse_set",       atbm_wifi_efuse_set},
    {"efuse_setall",       atbm_wifi_set_all_efuse},
    {"efuse_get",       atbm_wifi_efuse_get},
    {"efuse_getfirst",       atbm_wifi_efuse_getfirst},
    {"efuse_getall",       atbm_wifi_get_efuse_all_data},
    {"efuse_getfirstmac",       atbm_wifi_get_First_mac},
    {"efuse_getfreespace",       atbm_wifi_get_efuse_free_space},

	{"fwcmd",		atbm_wifi_fwcmd},
};
static int cmd_entry_check(wifi_cmd_entry_t * cmd_table, int argc, char **args)
{
	if(args[1]==NULL){
		return -1;
	}
    //find the right cmd name
	int cmd_cnt = sizeof(s_test_cmd_table) / sizeof(s_test_cmd_table[0]);
    for(int i = 0; i<cmd_cnt; i++)
    {
        if (strcmp(cmd_table[i].cmd_name, args[1]) == 0)
        {
            //find and execute entry function
            cmd_table[i].cmd_entry(argc, args);
            return 0;
        }
    }

    //set_error_no(ERROR_TYPE_INVALID_ARG);
    return -1;
}
static void wifi_test_help(){
	int cnt=sizeof(s_test_cmd_table)/sizeof(s_test_cmd_table[0]);
	printf("atbm_cmd_list:\n");
	for(int i=0;i<cnt;i++){
		printf("\t%s\n",s_test_cmd_table[i].cmd_name);
	}
}
static void atbm_wifi_test(int argc, char **args)
{
    struct atbmwifi_vif *priv = _atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,0);
	if(!priv){
		printf("wifi not inited!\n");
		return ;
	}
    if(cmd_entry_check(s_test_cmd_table, argc, args) != 0)
    {       
		wifi_test_help();
    }
	
}
MSH_CMD_EXPORT(atbm_wifi_test, altobeam wifi txrx test);

