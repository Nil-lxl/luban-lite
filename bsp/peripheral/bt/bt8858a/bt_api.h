#ifndef __BT_API_H__
#define __BT_API_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "bt_type.h"
#include "bt_config.h"
#include "aic_log.h"

#define 	        BT_MSG_MAX 	     512//BT MSG datalen
#define 	        BT_CMD_MAX 	     512// 队列有512个元素

//蓝牙音乐播放状态获取
typedef enum __BT_STATUS
{
    BT_STAT_IDLE = 0,
    BT_STAT_PLAY ,
    BT_STAT_PAUSE,
    BT_STAT_
} bt_status_t;

//蓝牙时间获取
typedef struct
{
   	__u8 year;
	__u8 month;
	__u8 day;
	__u8 hour;
	__u8 minute;
	__u8 second;
}bt_time_t;

/*******bt*******/
//蓝牙电话本获取
typedef struct
{
//////////////////电话本
	__u8 	phonebook_name[1000][64];			//名称
	__u8 	phonebook_number[1000][32];			//电话号码有两个以上的电话号码的用同一名称显示
	__s32 	cur_phone_total;						//最后一个
//////////////////通话记录
	//__u8 	phonerem_name[64][64];			//通话记录,现在废弃
	//__u8 	phonerem_number[64][32];			//电话号码有两个以上的电话号码的用同一名称显示
	//__u8 	phonerem_date[64][32];			////日期
	//__u8 	phonerem_type[64];			////类型////1:打出2:打进，3 :拒接
	//__s32 	cur_phonerem_total;

	__u8 	phoneout_name[32][64];			//打出
	__u8 	phoneout_number[32][32];		//电话号码有两个以上的电话号码的用同一名称显示
	__u8 	phoneout_date[32][32];			////日期
	__s32 	cur_phoneout_total;

	__u8 	phonein_name[32][64];			//打进
	__u8 	phonein_number[32][32];		//电话号码有两个以上的电话号码的用同一名称显示
	__u8 	phonein_date[32][32];			////日期
	__s32 	cur_phonein_total;

	__u8 	phonemiss_name[32][64];			//未接
	__u8 	phonemiss_number[32][32];		//电话号码有两个以上的电话号码的用同一名称显示
	__u8 	phonemiss_date[32][32];			////日期
	__s32 	cur_phonemiss_total;

}bt_phonebook_state;


//蓝牙音乐ID3信息获取
typedef struct BT_ID3_INFO
{
    __u8  title_name[256];//歌词
    __u8  artist_name[256];//歌名
    __u8  album_name[256];//歌手
    __u8  cur_time[32];    //当前播放时间
    __u8  total_time[32];  //播放总时间

}bt_ID3_info_t;

//车机蓝牙连接状态
typedef enum
{
	BT_STATE_INIT = 0,
	BT_STATE_NOTCONNECTE,
	BT_STATE_CONNECTED,
	BT_STATE_MAX,
}bt_connect_flag_t;

typedef enum
{
	BT_SPP_STATE_INIT = 0,
	BT_SPP_STATE_DISCONNECTED,
	BT_SPP_STATE_CONNECTED,
	BT_SPP_STATE_MAX,
}bt_spp_connect_flag_t;

typedef enum
{
	BT_BLE_STATE_INIT = 0,
	BT_BLE_STATE_DISCONNECTED,
	BT_BLE_STATE_CONNECTED,
	BT_BLE_STATE_MAX,
}bt_ble_connect_flag_t;

typedef enum
{
	BT_HID_STATE_INIT = 0,
	BT_HID_STATE_DISCONNECTED,
	BT_HID_STATE_CONNECTED,
	BT_HID_STATE_MAX,
}bt_hid_connect_flag_t;

//蓝牙电话状态
typedef enum
{
    BT_PHONE_INIT = 0,               	////初始化和一般情况
    BT_PHONE_INING,          	////电话打进
    BT_PHONE_OUTING,	  	////电话打出
    BT_PHONE_TALKINING,       ////通话中
    BT_PHONE_TALKOUTING,       ////通话中
    BT_MAX
}bt_phone_state_t;

//手机连接类型
typedef enum
{
    PHONE_ANDROID_AUTO,          	////android系统原生手机
    PHONE_CARPLAY ,                ////苹果手机
    PHONE_ANDROID_CARLIFE,	  	////android系统魔改手机
    PHONE_HARMONY,	  	////HARMONY手机
    PHONE_MAX
}bt_phone_type_t;


extern u8 	phone_macaddr_buf[12];//手机蓝牙MAC ADDR
extern u8 	phone_name_buf[24];//手机蓝牙名字
extern u8 	bt_hostname_buf[24];//车机蓝牙名字
extern u8 	bt_pincard_buf[4]; //蓝牙配对码
extern u8 	bt_macaddr_buf[12];//车机蓝牙MAC ADDR
extern u8	bt_callin_buf[64];//手机来电号码
extern u8   spp_rxdata[512];  //rfcomm 通讯后从手机接收透传的数据
extern u8   system_do_msg[BT_MSG_MAX];	//系统要处理的消息
extern bt_connect_flag_t 	 	bt_connect_flag; //蓝牙是否连接
extern bt_spp_connect_flag_t 	bt_spp_connect_flag;//蓝牙spp是否连接
extern bt_ble_connect_flag_t 	bt_ble_connect_flag; //蓝牙BLE是否连接
extern bt_phone_state_t			bt_phone_state;          //蓝牙电话状态
extern bt_phone_type_t			bt_phone_type;   //手机连接类型
extern bt_time_t                bt_time;
extern bt_phonebook_state		*bt_phonebook_para;
extern bt_ID3_info_t			*bt_ID3_info;


/******************************FMRX********************************************/
//本记录是BT的FM在不断电情况下保存数据，如果BT要断电，那么就要获取FMRX的LIST表在主控端记录
typedef struct {
    u16  freq;  //当前频点
    u8   ch_cur;//当前节目
    u8   ch_cnt;//总节目数
} bt_fmrx_info_t;

typedef struct {
    u8   ch_cnt;    //总节目数
    u16  freq[64];   //第一个节目从序号0开始
} bt_fmrx_list_t;

extern bt_fmrx_info_t			bt_fmrx_info;//某个节目的信息包括总节目数，当前节目号，以及对应的频点
extern bt_fmrx_list_t			bt_fmrx_list;//节目的频点列表




//system api

extern __s32  lowercase_2_uppercase(char *str);

//倒车控制
extern __s32 check_pev_key(void);

//接收蓝牙时间
extern __s32 sys_bt_set_time(u8 *time);

//获取蓝牙时间给应用
extern __s32 sys_bt_get_time_to_app(bt_time_t *time);

//设置时间给蓝牙
extern __s32 sys_bt_set_time_to_bt(bt_time_t *time);

//控制开关屏
extern void sys_bt_ctrl_lcd(u8 lcd_ctrl);

//控制开关屏
extern void sys_bt_ctrl_light(u8 light_ctrl);

//控制倒车影像
extern void sys_bt_ctrl_pev(u8 pev_ctrl);

//调用发送bt_phonename_get命令之后，以下函数获取电话本
extern __s32 sys_bt_set_phonebook(u8 *phonebook);

//清除本地获取电话本的BUFFER
extern void sys_bt_clear_phonebook(void);

//获取来电指示电话号码
extern __s32  sys_bt_get_phonenum(char *btstring);

//接收蓝牙FMRX信息执行的一些动作
extern __s32 sys_bt_fm_process(void);

//接收蓝牙信息执行的一些动作
extern __s32 sys_bt_dosomething(void);

//获取播放的蓝牙音乐的ID3等信息
extern __s32 sys_bt_set_music_info(void);

//获取播放的蓝牙音乐的状态(播放暂停)等信息
extern void sys_bt_set_music_status(u8 music_status);

//获取手机电话BT的状态
extern __s32 sys_bt_get_phone_state(void);

//bt返回设置手机电话BT的状态
extern __s32 sys_bt_set_phone_state(bt_phone_state_t phone_state);

//获取手机类型
__s32 sys_bt_get_phone_type(void);

//bt返回设置手机类型
__s32 sys_bt_set_phone_type(bt_phone_type_t phone_type);

//获取车机BT连接的状态，在CPU端的变量查询
extern __s32 sys_bt_is_connected(void);

//cpu-bt cmd api
extern void BtCmdSend (char *CmdStr,unsigned char len);

//获取车机蓝牙时间
extern __s32 bt_time_get(void);

//设置车机蓝牙时间
extern __s32 bt_time_set(void);

//获取大灯状态，需要车机蓝牙GPIO控制
extern __s32 bt_ill_get(void);

//获取车机蓝牙状态，通过发送UART命令返回的，第一次启动首先先要执行，
//后面只要不断电蓝牙就会自动返回连接状态给到CPU端变量，则通过sys_bt_is_connected查询
extern __s32 bt_state_get(void);

//获取手机连接的名字，这里默认都是车机列表第0个，由手机主动连接的车机蓝牙，
//车机蓝牙不做主动搜索蓝牙列表去连接手机
extern __s32 bt_phonename_get(void);

//获取车机蓝牙的名字小于24字节
extern __s32 bt_hostname_get(void);

//获取车机蓝牙的PIN码(4位数字配对码)，现在不用
extern __s32 bt_pincard_get(void);

///HFP状态查询
extern __s32 bt_get_hfpstate(void);

//蓝牙音乐播放停止，调用一次与上一次相反
extern __s32 bt_audio_play_pause(void);

//蓝牙音乐播放
extern __s32 bt_audio_play(void);

//蓝牙音乐暂停
extern __s32 bt_audio_pause(void);

//蓝牙音乐停止
extern __s32 bt_audio_stop(void);

//蓝牙音乐上一曲
extern __s32 bt_audio_prev(void);

//蓝牙音乐下一曲
extern __s32 bt_audio_next(void);

//获取蓝牙音乐的本地播放状态,必须先手机上播放蓝牙音乐传过来状态先,然后才能调用
extern __s32 bt_audio_get_status(void);

//获取蓝牙音乐的本地播放状态,必须先手机上播放蓝牙音乐传过来状态先,然后才能调用
extern __s32 bt_audio_set_status(uint8_t play_status);

//获取手机电话本，通话记录等，记录在bt_phonebook_state数据结构里面
extern __s32 bt_phonebook_get(void);

//切换音频通道
extern __s32 bt_audio_change(void);

//强行切到手机上
extern __s32 bt_audio_to_phone(void);

//强行切到车机上
extern __s32 bt_audio_to_machine(void);

//通知bt power off CPU
//BT做电源管理时，才可以开关CPU电源
extern __s32 bt_power_off(void);

//来电接听
extern __s32 bt_phone_incoming(void);

//一次性发出电话号码。
extern __s32 bt_phone_call(char *phonenumber,__s32 len);

//挂断电话
extern __s32 bt_phone_break(void);

//拒接来电
extern __s32 bt_phone_reject(void);

//重拨电话
extern __s32 bt_phone_recall(void);

//BT是否已连接 ?
extern __s32  bt_is_connected(void);

//重新连接BT
extern __s32 bt_connect_set(void);

//断开连接BT
extern __s32 bt_disconnect_set(void);

//获取车机蓝牙MAC地址
extern __s32 bt_macaddr_get(void);

//EQ用户模式
extern __s32 bt_eq_user_mode(__u8 index, __u8 gain);

//EQ流行模式
extern __s32 bt_eq_pop_mode(__u8 index, __u8 gain);

//EQ摇滚模式
extern __s32 bt_eq_rock_mode(__u8 index, __u8 gain);

//EQ爵士模式
extern __s32 bt_eq_jazz_mode(__u8 index, __u8 gain);

//EQ经典模式
extern __s32 bt_eq_classic_mode(__u8 index, __u8 gain);

//EQ柔和模式
extern __s32 bt_eq_gentle_mode(__u8 index, __u8 gain);

//音效前移
extern __s32 bt_dac_vol_front(__u8 vol);

//音效后移
extern __s32 bt_dac_vol_behind(__u8 vol);

//音效左移
extern __s32 bt_dac_vol_left(__u8 vol);

//音效右移
extern __s32 bt_dac_vol_right(__u8 vol);

//vol+
extern __s32 bt_vol_add(void);

//vol-
extern __s32 bt_vol_dec(void);

//静音
extern __s32 bt_vol_mute(void);

//解除静音
extern __s32 bt_vol_unmute(void);

/*********************SPP COMM******************************/
//"AT#"打头的指令透传通道
//发送透传数据
extern __s32 bt_spp_tx_data (char *CmdStr,unsigned char len);
//接收透传数据
extern __s32 bt_spp_rx_data(char *CmdStr,unsigned char len);

extern __s32 bt_spp_rfcomm_is_connected(void);

extern __s32 bt_spp_rfcomm_connect(void);

extern __s32 bt_spp_rfcomm_disconnect(void);

//RFCOMM连接成功后，AT#SG加上第三方手机互联库发过来的形成数据串，转为AT##再通过RFCOMM发送给手机
extern __s32 bt_spp_tx_data_rfcomm(char *CmdStr,unsigned char len);

//RFCOMM连接成功后，手机上层发过来的RFCOMM数据串，后面可发送第三方手机互联库
extern __s32 bt_spp_rx_data_rfcomm(char *CmdStr,unsigned char len);

/*********************BLE COMM******************************/

//extern __s32 bt_ble_tx_data(char *CmdStr,unsigned char len);

//extern __s32 bt_ble_rx_data(char *CmdStr,unsigned char len);

//extern __s32 bt_ble_rfcomm_is_connected(void);

//extern __s32 bt_ble_rfcomm_is_connect(void);

//extern __s32 bt_ble_rfcomm_disconnect(void);

//extern __s32 bt_ble_tx_data_rfcomm(char *CmdStr,unsigned char len);

//extern __s32 bt_ble_rx_data_rfcomm(char *CmdStr,unsigned char len);

/*********************HID COMM******************************/

//"AT#@"打头的指令透传通道
//发送数据
extern __s32 bt_hid_tx_data(char *CmdStr,unsigned char len);

//接收bt数据
extern __s32 bt_hid_rx_data(char *CmdStr,unsigned char len);

//HID是否连接
extern __s32 bt_hid_is_connected(void);

// 注册HID连接callback
extern void bt_hid_request_connect_cb(void *cb(int status));

//连接HID
extern __s32 bt_hid_connect(void);

//断开HID
extern __s32 bt_hid_disconnect(void);

//设置手机光标指针的坐标位置给蓝牙
extern __s32 bt_hid_set_cursor_pos(s16 x_pos,s16 y_pos);

//发送触摸滑动点坐标相对变化值(需要减去上一次坐标值)给bt
//action 0:指针移动；1:鼠标左键按下；2:鼠标左键抬起；3:鼠标中键按下；4:鼠标中键抬起；
//5:鼠标右键按下；6:鼠标右键抬起；
extern __s32 bt_hid_set_cursor_xy(u8 action,s16 x_diff,s16 y_diff);

//重置手机光标指针的坐标位置给蓝牙，这个时候的光标位置就是bt_hid_set_cursor_pos函数设置的坐标
extern __s32 bt_hid_reset_cursor_pos(void);

//测试鼠标滑动点击效果给bt
extern __s32 bt_hid_mouse_test(void);

/*********************FMRX COMM******************************/
//搜台
extern __s32 bt_fmrx_search (void);

//获取节目频道号信息，如当前频点，总节目数
extern __s32 bt_fmrx_get_info (void);

//获取节目频道列表
extern __s32 bt_fmrx_get_list (void);

//获取总节目数
extern __s32 bt_fmrx_get_ch_cnt (void);

//播放节目号
extern __s32 bt_fmrx_set_ch (__u8 ch);

//播放频点
extern __s32 bt_fmrx_set_freq (__u16 freq);

//切节目号，dir为1往108M方向切，0往78.5M方向切
extern __s32 bt_fmrx_switch_channel (__s8 dir);

//切频点，dir为1往108M方向切，0往78.5M方向切
extern __s32 bt_fmrx_switch_freq (__s8 dir);

//获取节目号对应的频点
__s32 bt_fmrx_get_ch_to_freq (__u8 ch);

/*********************BT MOUDLE******************************/

//切换蓝牙的应用,BT启动默认蓝牙应用//
//index值如下:
//0x41: FMRX;   0x42:AUX2;   0x43:BT;    0x44:MUSIC
extern __s32 bt_switch_app(u8 index);

//字符转换成十六进制显示，转换后长度减半 srcLen/2
extern void bt_StrToHex(unsigned char *pbDest, unsigned char *pbSrc, int srcLen);
//十六进制转换成字符显示，转换后长度加倍 srcLen*2
extern void bt_HexToStr(char *pszDest, char *pbSrc, int srcLen);


//bt_init 初始化内部数据结构和创建线程/定时器
extern __s32 bt_init(void);
//bt_exit 初始化内部数据结构和释放线程/定时器
extern __s32 bt_exit(void);

#ifdef __cplusplus
}
#endif

#endif


