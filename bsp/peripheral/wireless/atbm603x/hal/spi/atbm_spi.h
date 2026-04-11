/**************************************************************************************************************
 * altobeam RTOS spi.h
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#ifndef ATBMWIFI__HWIO_H_INCLUDED
#define ATBMWIFI__HWIO_H_INCLUDED

/* extern */ struct atbmwifi_common;



#define M2W_INTERRPUTE_EN 0x16100000
#define W2M_INTERRPUTE_MASK 0x16100004


#define HI_IND_BASE		 0x0800
/* Message bases */
#define HI_REQ_BASE		 0x0000
#define HI_CNF_BASE		 0x0400

#define SYS_MAX_INP_MSG_SIZE        1632    /* Max Size of Input Buffers */

#define HIF_OFFSET_A_INPUT(_desc,mask)     ((8*((_desc)&(mask)))+AHB_INPUT_POINT_BASE)
#define HIF_OFFSET_B_INPUT(_desc,mask)     ((8*((_desc)&(mask)) + 4)+AHB_INPUT_POINT_BASE)

#define HIF_OFFSET_A_OUTPUT(_desc,mask)     ((8*((_desc)&(mask)))+AHB_OUTPUT_POINT_BASE)
#define HIF_OFFSET_B_OUTPUT(_desc,mask)     ((8*((_desc)&(mask)) + 4)+AHB_OUTPUT_POINT_BASE)



//#define HI_MSG_ID_MASK	      	0x1FFF
//#define HI_MSG_SEQ_RANGE	    0x0007
//#define HI_GET_MSG_SEQ(_id)	 		 (((_id) >> 13)&HI_MSG_SEQ_RANGE)
//#define HI_PUT_MSG_SEQ(_id, _seq)    ((_id) | (atbm_uint16)((_seq) << 13))
//#define HI_GET_MSG_ID(_id)	 		 ((_id) &HI_MSG_ID_MASK)

//#define AHB_MAKE_DESC_B(len,MsgId)		(((MsgId)<<16)|(len))
//#define AHB_GET_LEN(b)			((b)&0xffff)
//#define AHB_GET_MSG(b)			((b)>>16)
//#define AHB_GET_MSG_ID(b)		(AHB_GET_MSG(b)&HI_MSG_ID_MASK)
//#define AHB_GET_MSG_SEQ(b)		HI_GET_MSG_SEQ(AHB_GET_MSG(b))



#define RATE_INDEX_B_1M           0
#define RATE_INDEX_B_2M           1
#define RATE_INDEX_B_5_5M         2
#define RATE_INDEX_B_11M          3
#define RATE_INDEX_PBCC_22M       4     // not supported/unused
#define RATE_INDEX_PBCC_33M       5     // not supported/unused
#define RATE_INDEX_A_6M           6
#define RATE_INDEX_A_9M           7
#define RATE_INDEX_A_12M          8
#define RATE_INDEX_A_18M          9
#define RATE_INDEX_A_24M          10
#define RATE_INDEX_A_36M          11
#define RATE_INDEX_A_48M          12
#define RATE_INDEX_A_54M          13
#define RATE_INDEX_N_6_5M         14
#define RATE_INDEX_N_13M          15
#define RATE_INDEX_N_19_5M        16
#define RATE_INDEX_N_26M          17
#define RATE_INDEX_N_39M          18
#define RATE_INDEX_N_52M          19
#define RATE_INDEX_N_58_5M        20
#define RATE_INDEX_N_65M          21




int atbmwifi_data_read(struct atbmwifi_common *hw_priv, atbm_uint32 addr,
		     atbm_void *buf, atbm_size_t buf_len);
int atbmwifi_data_write(struct atbmwifi_common *hw_priv, atbm_uint32 addr,
		       atbm_void *buf, atbm_size_t buf_len);
int atbmwifi_reg_read_32(struct atbmwifi_common *hw_priv,
				     atbm_uint32 addr, atbm_uint32 *val);
int atbmwifi_reg_write_32(struct atbmwifi_common *hw_priv,
				      atbm_uint32 addr, atbm_uint32 val);


int atbmwifi_wakeup_lmac(struct atbmwifi_common *hw_priv);
int atbmwifi_sleep_lmac(struct atbmwifi_common *hw_priv);
int atbmwifi_hwio_init(struct atbmwifi_common *hw_priv);

int atbmwifi_cansleep_lmac(struct atbmwifi_common *hw_priv);
int atbmwifi_get_wakeup_status(struct atbmwifi_common *hw_priv);


#endif /* ATBMWIFI__HWIO_H_INCLUDED */
