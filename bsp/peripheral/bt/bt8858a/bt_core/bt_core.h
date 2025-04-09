#ifndef __BT_CORE_H__
#define __BT_CORE_H__

#ifdef __cplusplus
extern "C" {
#endif


//将小写字母转换成大小字母
__s32  lowercase_2_uppercase(char *str);

//cpu-bt cmd api
void BtCmdSend (char *CmdStr,unsigned char len);

//获取车机蓝牙时间
__s32 bt_time_get(void);

//设置车机蓝牙时间
__s32 bt_time_set(void);

//获取大灯状态，需要车机蓝牙GPIO控制
__s32 bt_ill_get(void);

//获取车机蓝牙状态，通过发送UART命令返回的，第一次启动首先先要执行，
//后面只要不断电蓝牙就会自动返回连接状态给到CPU端变量，则通过sys_bt_is_connected查询
__s32 bt_state_get(void);

//获取手机连接的名字，这里默认都是车机列表第0个，由手机主动连接的车机蓝牙，
//车机蓝牙不做主动搜索蓝牙列表去连接手机
__s32 bt_phonename_get(void);

//获取车机蓝牙的名字小于24字节
__s32 bt_hostname_get(void);

//获取车机蓝牙的PIN码(4位数字配对码)，现在不用
__s32 bt_pincard_get(void);

///HFP状态查询
__s32 bt_get_hfpstate(void);

//蓝牙音乐播放停止，调用一次与上一次相反
__s32 bt_audio_play_pause(void);

//蓝牙音乐播放
__s32 bt_audio_play(void);

//蓝牙音乐暂停
__s32 bt_audio_pause(void);

//蓝牙音乐停止
__s32 bt_audio_stop(void);

//蓝牙音乐上一曲
__s32 bt_audio_prev(void);

//蓝牙音乐下一曲
__s32 bt_audio_next(void);

//获取蓝牙音乐的本地播放状态,必须先手机上播放蓝牙音乐传过来状态先,然后才能调用
__s32 bt_audio_get_status(void);

//获取蓝牙音乐的本地播放状态,必须先手机上播放蓝牙音乐传过来状态先,然后才能调用
__s32 bt_audio_set_status(uint8_t play_status);

//获取手机电话本，通话记录等，记录在bt_phonebook_state数据结构里面
__s32 bt_phonebook_get(void);

//切换音频通道
__s32 bt_audio_change(void);

//强行切到手机上
__s32 bt_audio_to_phone(void);

//强行切到车机上
__s32 bt_audio_to_machine(void);

//通知bt power off CPU
//BT做电源管理时，才可以开关CPU电源
__s32 bt_power_off(void);

//来电接听
__s32 bt_phone_incoming(void);

//一次性发出电话号码。
__s32 bt_phone_call(char *phonenumber,__s32 len);

//挂断电话
__s32 bt_phone_break(void);

//拒接来电
__s32 bt_phone_reject(void);

//重拨电话
__s32 bt_phone_recall(void);

//BT是否已连接 ?
__s32  bt_is_connected(void);

//重新连接BT
__s32 bt_connect_set(void);

//断开连接BT
__s32 bt_disconnect_set(void);

//获取车机蓝牙MAC地址
__s32 bt_macaddr_get(void);

//EQ用户模式
__s32 bt_eq_user_mode(__u8 index, __u8 gain);

//EQ流行模式
__s32 bt_eq_pop_mode(__u8 index, __u8 gain);

//EQ摇滚模式
__s32 bt_eq_rock_mode(__u8 index, __u8 gain);

//EQ爵士模式
__s32 bt_eq_jazz_mode(__u8 index, __u8 gain);

//EQ经典模式
__s32 bt_eq_classic_mode(__u8 index, __u8 gain);

//EQ柔和模式
__s32 bt_eq_gentle_mode(__u8 index, __u8 gain);

//音效前移
__s32 bt_dac_vol_front(__u8 vol);

//音效后移
__s32 bt_dac_vol_behind(__u8 vol);

//音效左移
__s32 bt_dac_vol_left(__u8 vol);

//音效右移
__s32 bt_dac_vol_right(__u8 vol);

//vol+
__s32 bt_vol_add(void);

//vol-
__s32 bt_vol_dec(void);

//静音
__s32 bt_vol_mute(void);

//解除静音
__s32 bt_vol_unmute(void);

/*********************SPP COMM******************************/
//"AT#"打头的指令透传通道
//发送透传数据
extern __s32 bt_spp_tx_data (char *CmdStr,unsigned char len);
//接收透传数据
extern __s32 bt_spp_rx_data(char *CmdStr,unsigned char len);

//判断SPP RFCOMM是否连接成功
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

//extern __s32 bt_ble_rfcomm_connect(void);

//extern __s32 bt_ble_rfcomm_disconnect(void);

//extern __s32 bt_ble_tx_data_rfcomm(char *CmdStr,unsigned char len);

//extern __s32 bt_ble_rx_data_rfcomm(char *CmdStr,unsigned char len);

/*********************HID COMM******************************/

//"AT#@"打头的指令透传通道
//发送数据
 __s32 bt_hid_tx_data(char *CmdStr,unsigned char len);

//接收bt数据
 __s32 bt_hid_rx_data(char *CmdStr,unsigned char len);

//HID是否连接
__s32 bt_hid_is_connected(void);

//连接HID
__s32 bt_hid_connect(void);

//断开HID
__s32 bt_hid_disconnect(void);

//设置手机光标指针的坐标位置给蓝牙
__s32 bt_hid_set_cursor_pos(s16 x_pos,s16 y_pos);

//发送触摸滑动点坐标相对变化值(需要减去上一次坐标值)给bt
//action 0:指针移动；1:鼠标左键按下；2:鼠标左键抬起；3:鼠标中键按下；4:鼠标中键抬起；
//5:鼠标右键按下；6:鼠标右键抬起；
__s32 bt_hid_set_cursor_xy(u8 action,s16 x_diff,s16 y_diff);

//重置手机光标指针的坐标位置给蓝牙，这个时候的光标位置就是bt_hid_set_cursor_pos函数设置的坐标
__s32 bt_hid_reset_cursor_pos(void);

//测试点击触摸滑动效果给bt
__s32 bt_hid_mouse_test(void);


/*********************FMRX COMM******************************/
//搜台
__s32 bt_fmrx_search (void);

//获取节目频道号信息，如当前频点，总节目数
__s32 bt_fmrx_get_info (void);

//获取节目频道列表
__s32 bt_fmrx_get_list (void);

//获取总节目数
__s32 bt_fmrx_get_ch_cnt (void);

//播放节目号
__s32 bt_fmrx_set_ch (__u8 ch);

//播放频点
__s32 bt_fmrx_set_freq (__u16 freq);

//切节目号，dir为1往108M方向切，0往78.5M方向切
__s32 bt_fmrx_switch_channel (__s8 dir);

//切频点，dir为1往108M方向切，0往78.5M方向切
__s32 bt_fmrx_switch_freq (__s8 dir);

//获取节目号对应的频点
__s32 bt_fmrx_get_ch_to_freq (__u8 ch);

//切换蓝牙的应用,BT启动默认蓝牙应用//
//index值如下:
//0x41: FMRX;   0x42:AUX2;   0x43:BT;    0x44:MUSIC
__s32 bt_switch_app(u8 index);

//字符转换成十六进制显示，转换后长度减半 srcLen/2
void bt_StrToHex(unsigned char *pbDest, unsigned char *pbSrc, int srcLen);
//十六进制转换成字符显示，转换后长度加倍 srcLen*2
void bt_HexToStr(char *pszDest, char *pbSrc, int srcLen);


#ifdef __cplusplus
}
#endif

#endif /* __BT_CORE_H__ */
