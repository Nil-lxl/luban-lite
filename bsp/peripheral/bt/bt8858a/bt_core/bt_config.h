#ifndef __BT_CONFIG_H__
#define __BT_CONFIG_H__

//#define   RTOS   1
#define    RTT   1
//#define LINUX   1

#define openmode    1       //操作串口方式 1:open device;   0:fs open bus 

#define runmode     1       //接收数据运行方式 1:thread线程   0: timer定时器

#define comm_mode       1//接收字段方式 1:\r\n, 2:\\r\\n,3: 0xff


#endif
