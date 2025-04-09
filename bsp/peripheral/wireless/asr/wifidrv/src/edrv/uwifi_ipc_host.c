/**
 ******************************************************************************
 *
 * @file ipc_host.c
 *
 * @brief IPC module.
 *
 * Copyright (C) ASR
 *
 ******************************************************************************
 */

/*
 * INCLUDE FILES
 ******************************************************************************
 */
#include <stdio.h>
#include "asr_dbg.h"
#include "asr_config.h"
#include "uwifi_ipc_host.h"
#include "uwifi_ops_adapter.h"
#include "uwifi_hif.h"
#include "uwifi_sdio.h"
#include "uwifi_txq.h"
#include "uwifi_msg.h"
//extern int tx_aggr;  //8 will error
extern int rx_aggr;
//extern int rx_aggr_max_num;
extern int lalalaen;
extern uint32_t asr_rx_pkt_cnt;

/*
 * FUNCTIONS DEFINITIONS
 ******************************************************************************
 */
/**
 ******************************************************************************
 * @brief Handle the reception of a Rx Descriptor
 *
 ******************************************************************************
 */
static inline void ipc_host_rxdata_handler(struct ipc_host_env_tag *env,struct sk_buff *skb)
{
    env->cb.recv_data_ind(env->pthis,(void *)skb);
}

void asr_rx_to_os_task(struct asr_hw *asr_hw)
{
    struct sk_buff *skb;
    while (!skb_queue_empty(&asr_hw->rx_to_os_skb_list) && (skb = (struct sk_buff *)skb_dequeue(&asr_hw->rx_to_os_skb_list)))
    {
        //dbg(D_DBG, D_UWIFI_CTRL, "%s: asr_hw->sta_vif_idx=%d,asr_hw->monitor_vif_idx=%d,asr_hw->ap_vif_idx=%d\r\n",__func__,asr_hw->sta_vif_idx,asr_hw->monitor_vif_idx,asr_hw->ap_vif_idx);
        if (asr_hw->sta_vif_idx != 0xFF || asr_hw->monitor_vif_idx != 0xFF || asr_hw->ap_vif_idx != 0xFF)
            ipc_host_rxdata_handler(asr_hw->ipc_env,skb);
        else {

            #ifndef SDIO_DEAGGR
            //if asr_hw->sta_vif_idx is 0xFF, directly drop the received data and not process it, just push it back to rx_sk_list
            skb_queue_tail(&asr_hw->rx_data_sk_list, skb);
            #else
            memset(skb->data, 0, asr_hw->ipc_env->rx_bufsz_sdio_deagg);
            // Add the sk buffer structure in the table of rx buffer
            skb_queue_tail(&asr_hw->rx_sk_sdio_deaggr_list, skb);
            #endif
        }
    }

}

/**
 ******************************************************************************
 */
static inline void ipc_host_msg_handler(struct ipc_host_env_tag *env,struct sk_buff *skb)
{
    env->cb.recv_msg_ind(env->pthis,(void *)skb);
}

/**
 ******************************************************************************
 */
static inline void ipc_host_dbg_handler(struct ipc_host_env_tag *env,struct sk_buff *skb)
{
    env->cb.recv_dbg_ind(env->pthis,(void *)skb);
}


// deaggr skb here, when enable ooo, skb contain data and rxdesc.
#ifdef SDIO_DEAGGR
int rx_ooo_upload = 1;   // enable ooo or not.
u32 pending_data_cnt;
u32 direct_fwd_cnt;
u32 rxdesc_refwd_cnt;
u32 rxdesc_del_cnt;
u32 min_deaggr_free_cnt = 0xFF;
int asr_total_rx_sk_deaggrbuf_cnt;
int asr_wait_rx_sk_deaggrbuf_cnt;

int asr_wait_rx_sk_deaggrbuf(struct asr_hw *asr_hw)
{
    int wait_mem_cnt = 0;
    int ret = 0;

    asr_wait_rx_sk_deaggrbuf_cnt++;

    do {
        wait_mem_cnt++;
        asr_rtos_delay_milliseconds(1);

    } while (skb_queue_empty(&asr_hw->rx_sk_sdio_deaggr_list) && (wait_mem_cnt < 100));

    if (skb_queue_empty(&asr_hw->rx_sk_sdio_deaggr_list))
        ret = -1;

    return ret;
}

int to_os_less8_cnt;
int to_os_less16_cnt;
int to_os_more16_cnt;
int max_to_os_delay_cnt;
u8 asr_sdio_deaggr(struct asr_hw *asr_hw,struct sk_buff *skb_head,u8 aggr_num)
{
    struct sk_buff *skb = skb_head;
    struct host_rx_desc *host_rx_desc = (struct host_rx_desc*)skb->data;

    int msdu_offset = 0;
    int frame_len = 0;
    u16 sdio_rx_len = 0;
    int num = aggr_num;
    bool need_wakeup_rxos_thread = false;
    int rx_skb_to_os_cnt;
    int rx_skb_to_os_cnt_delay;

    #ifdef CFG_OOO_UPLOAD
    bool forward_skb = false;
    #endif

#ifdef CONFIG_ASR_KEY_DBG
    int free_deaggr_skb_cnt;
#endif

    if (aggr_num > 8) {
        dbg(D_ERR, D_UWIFI_CTRL, "error:aggr_num=%u.",aggr_num);
        num = 0;
    }

    while(num--)
    {
            #ifdef CFG_OOO_UPLOAD
            if (host_rx_desc->id == HOST_RX_DATA_ID)
            #endif
            {
                    // get data skb info.
                    sdio_rx_len = host_rx_desc->sdio_rx_len;
                    msdu_offset = host_rx_desc->pld_offset;
                    frame_len   = host_rx_desc->frmlen;

                    if ((msdu_offset + frame_len  > IPC_RXMSGBUF_SIZE) || (host_rx_desc->totol_frmlen > WLAN_AMSDU_RX_LEN))
                        dbg(D_ERR, D_UWIFI_CTRL, "unlikely host_rx_desc : %d + %d < %d , %d < %d  \n", msdu_offset,frame_len,IPC_RXMSGBUF_SIZE,
                                                                                                     host_rx_desc->totol_frmlen,WLAN_AMSDU_RX_LEN);

                    if (!sdio_rx_len || !msdu_offset || !frame_len) {
                        dbg(D_ERR, D_UWIFI_CTRL, "rx data error:%u,",aggr_num,sdio_rx_len,msdu_offset,frame_len);
                        break;
                    }

                    asr_total_rx_sk_deaggrbuf_cnt++;
                    if (skb_queue_empty(&asr_hw->rx_sk_sdio_deaggr_list)) {
                        // caculate pending list and to os list.

                        rx_skb_to_os_cnt   = skb_queue_len(&asr_hw->rx_to_os_skb_list);
                        if (rx_skb_to_os_cnt <8)
                            to_os_less8_cnt++;
                        else if (rx_skb_to_os_cnt < 16)
                            to_os_less16_cnt++;
                        else
                            to_os_more16_cnt++;

                        asr_wait_rx_sk_deaggrbuf(asr_hw);

                        rx_skb_to_os_cnt_delay  = skb_queue_len(&asr_hw->rx_to_os_skb_list);

                        if ((rx_skb_to_os_cnt > rx_skb_to_os_cnt_delay) && ((rx_skb_to_os_cnt - rx_skb_to_os_cnt_delay) > max_to_os_delay_cnt))
                               max_to_os_delay_cnt = (rx_skb_to_os_cnt - rx_skb_to_os_cnt_delay);
                    }

                    if (!skb_queue_empty(&asr_hw->rx_sk_sdio_deaggr_list)) {

                        skb = skb_dequeue(&asr_hw->rx_sk_sdio_deaggr_list);
                        memcpy(skb->data, host_rx_desc, msdu_offset + frame_len);

                        #ifdef CFG_OOO_UPLOAD
                        // ooo refine : check if pkt is forward.
                        if (rx_ooo_upload) {
                                struct host_rx_desc *host_rx_desc = (struct host_rx_desc *)skb->data;
                                //dbg(D_ERR, D_UWIFI_CTRL, "RX_DATA_ID : host_rx_desc =%p , rx_status=0x%x \n", host_rx_desc,host_rx_desc->rx_status);

                                if (host_rx_desc && (host_rx_desc->rx_status & (RX_STAT_FORWARD | RX_STAT_MONITOR))){
                                    forward_skb = true;
                                    direct_fwd_cnt++;
                                } else if (host_rx_desc && (host_rx_desc->rx_status & RX_STAT_ALLOC)){
                                    skb_queue_tail(&asr_hw->rx_pending_skb_list, skb);
                                    forward_skb = false;
                                    pending_data_cnt++;

                                    if (lalalaen)
                                        dbg(D_ERR, D_UWIFI_CTRL, "RX_DATA_ID pending: host_rx_desc =%p , rx_status=0x%x (%d %d %d %d)\n", host_rx_desc,
                                                                    host_rx_desc->rx_status,
                                                                    host_rx_desc->sta_idx,
                                                                    host_rx_desc->tid,
                                                                    host_rx_desc->seq_num,
                                                                    host_rx_desc->fn_num);
                                } else {
                                    dbg(D_ERR, D_UWIFI_CTRL, "unlikely: host_rx_desc =%p , rx_status=0x%x, flags=0x%x \n", host_rx_desc,host_rx_desc->rx_status,host_rx_desc->flags);
                                    {
                                        memset(skb->data, 0, asr_hw->ipc_env->rx_bufsz_sdio_deagg);
                                        // Add the sk buffer structure in the table of rx buffer
                                        skb_queue_tail(&asr_hw->rx_sk_sdio_deaggr_list, skb);
                                    }
                                }

                        } else {
                            forward_skb = true;
                        }

                        if (forward_skb)
                        #endif  //CFG_OOO_UPLOAD
                        {
                            skb_queue_tail(&asr_hw->rx_to_os_skb_list, skb);
                            need_wakeup_rxos_thread = true;
                        }

                    }else {
                        dbg(D_ERR, D_UWIFI_CTRL, "%s: unlikely, runout deaggr skb,just skip (%d %d %d) %d \n", __func__,
                                                          msdu_offset,frame_len,host_rx_desc->totol_frmlen,IPC_RXBUF_CNT_SDIO_DEAGG);
                    }
            }
            #ifdef CFG_OOO_UPLOAD
            else if ((host_rx_desc->id == HOST_RX_DESC_ID) && rx_ooo_upload) {
                // ooo refine: parse ooo rxdesc
                //       RX_STAT_FORWARD : move pkt from rx_pending_skb_list to rx_to_os_skb_list and trigger
                //       RX_STAT_DELETE  : move pkt from rx_pending_skb_list and free.

                struct ipc_ooo_rxdesc_buf_hdr *ooo_rxdesc = (struct ipc_ooo_rxdesc_buf_hdr *)host_rx_desc;
                uint8_t rxu_stat_desc_cnt = ooo_rxdesc->rxu_stat_desc_cnt;
                struct rxu_stat_val *rxu_stat_handle;
                uint8_t * start_addr = (void *)ooo_rxdesc + sizeof(struct ipc_ooo_rxdesc_buf_hdr);
                int forward_pending_skb_cnt = 0;
                struct sk_buff *skb_pending;
                struct sk_buff *pnext;
                struct host_rx_desc *host_rx_desc_pending = NULL;
                uint16_t ooo_match_cnt = 0;

                sdio_rx_len = ooo_rxdesc->sdio_rx_len;

                if (lalalaen)
                    dbg(D_ERR, D_UWIFI_CTRL, "RX_DESC_ID : ooo_rxdesc =%p , rxu_stat_desc_cnt=%d ,start_addr=%p,hdr_len=%u\n", ooo_rxdesc,rxu_stat_desc_cnt,start_addr,
                                                             (unsigned int)sizeof(struct ipc_ooo_rxdesc_buf_hdr));
                if (ooo_rxdesc && rxu_stat_desc_cnt) {

                    while (rxu_stat_desc_cnt > 0){
                           rxu_stat_handle = (struct rxu_stat_val *)start_addr;

                            if (lalalaen)
                                dbg(D_ERR, D_UWIFI_CTRL, "RX_DESC_ID search: ooo_rxdesc =%p cnt=%d, rxu_stat_handle status=0x%x (%d %d %d %d)\n", ooo_rxdesc,rxu_stat_desc_cnt,
                                                        rxu_stat_handle->status,
                                                        rxu_stat_handle->sta_idx,
                                                        rxu_stat_handle->tid,
                                                        rxu_stat_handle->seq_num,
                                                        rxu_stat_handle->amsdu_fn_num);

                           // search and get from asr_hw->rx_pending_skb_list.
                           skb_queue_walk_safe(&asr_hw->rx_pending_skb_list, skb_pending, pnext) {
                               host_rx_desc_pending = (struct host_rx_desc *)skb_pending->data;
                               ooo_match_cnt = 0;

                                if (lalalaen)
                                    dbg(D_ERR, D_UWIFI_CTRL, "RX_DESC_ID walk: pending host_rx_desc rx_status=0x%x (%d %d %d %d)\n",
                                                        host_rx_desc_pending->rx_status,
                                                        host_rx_desc_pending->sta_idx,
                                                        host_rx_desc_pending->tid,
                                                        host_rx_desc_pending->seq_num,
                                                        host_rx_desc_pending->fn_num);

                               if ((host_rx_desc_pending->sta_idx == rxu_stat_handle->sta_idx) &&
                                   (host_rx_desc_pending->tid      == rxu_stat_handle->tid) &&
                                   (host_rx_desc_pending->seq_num == rxu_stat_handle->seq_num) &&
                                   (host_rx_desc_pending->fn_num  <= rxu_stat_handle->amsdu_fn_num)) {

                                   skb_unlink(skb_pending, &asr_hw->rx_pending_skb_list);
                                   ooo_match_cnt++;

                                   if (lalalaen)
                                       dbg(D_ERR, D_UWIFI_CTRL, "RX_DESC_ID hit: ooo_rxdesc =%p cnt=%d, rx_status=0x%x (%d %d %d %d)\n", ooo_rxdesc,rxu_stat_desc_cnt,
                                                        host_rx_desc_pending->rx_status,
                                                        host_rx_desc_pending->sta_idx,
                                                        host_rx_desc_pending->tid,
                                                        host_rx_desc_pending->seq_num,
                                                        host_rx_desc_pending->fn_num);

                                   if (rxu_stat_handle->status & RX_STAT_FORWARD){
                                       host_rx_desc_pending->rx_status |= RX_STAT_FORWARD;
                                       skb_queue_tail(&asr_hw->rx_to_os_skb_list, skb_pending);
                                       forward_pending_skb_cnt++;
                                       rxdesc_refwd_cnt++;
                                   } else if (rxu_stat_handle->status & RX_STAT_DELETE) {
                                       {
                                           memset(skb_pending->data, 0, asr_hw->ipc_env->rx_bufsz_sdio_deagg);
                                           // Add the sk buffer structure in the table of rx buffer
                                           skb_queue_tail(&asr_hw->rx_sk_sdio_deaggr_list, skb_pending);
                                       }

                                        rxdesc_del_cnt++;
                                   } else {
                                       dbg(D_ERR, D_UWIFI_CTRL, "unlikely: status =0x%x\n", rxu_stat_handle->status);
                                   }
                               }
                               if (ooo_match_cnt == (rxu_stat_handle->amsdu_fn_num + 1))
                                   break;
                           }

                           //get next rxu_stat_val
                           start_addr += sizeof(struct rxu_stat_val);
                           rxu_stat_desc_cnt--;
                    }

                    // trigger
                    if (forward_pending_skb_cnt) {
                        need_wakeup_rxos_thread = true;
                    }
                }
            }else {
                dbg(D_ERR, D_UWIFI_CTRL, "id = 0x%x, not support! \n", host_rx_desc->id);
            }
            #endif    //CFG_OOO_UPLOAD

            if(num)
            {
                // next rx data or rxdesc.
                host_rx_desc = (struct host_rx_desc*)((u8*)host_rx_desc + sdio_rx_len);
            }
    }

    #ifdef CONFIG_ASR_KEY_DBG
    free_deaggr_skb_cnt  = skb_queue_len(&asr_hw->rx_sk_sdio_deaggr_list);
    if (free_deaggr_skb_cnt < min_deaggr_free_cnt)
        min_deaggr_free_cnt = free_deaggr_skb_cnt;
    #endif

    if (need_wakeup_rxos_thread) {
        //set_bit(ASR_FLAG_RX_TO_OS_BIT, &asr_hw->ulFlag);
        //wake_up_interruptible(&asr_hw->waitq_rx_to_os_thead);
        uwifi_msg_rx_to_os_queue();
    }

    // deagg sdio buf done, free skb_head here.
    memset(skb_head->data, 0, asr_hw->ipc_env->rx_bufsz);
    // Add the sk buffer structure in the table of rx buffer
    skb_queue_tail(&asr_hw->rx_data_sk_list, skb_head);

    return 0;
}
#endif

int ipc_host_process_rx_sdio(int port,struct asr_hw *asr_hw,struct sk_buff *skb,u8 aggr_num)
{
    u8 type;

    if (port == SDIO_PORT_IDX(0))   //msg
    {
        ipc_host_msg_handler(asr_hw->ipc_env,skb);
        type = HIF_RX_MSG;
    }
    else if (port == SDIO_PORT_IDX(1))  //log
    {
        ipc_host_dbg_handler(asr_hw->ipc_env, skb);
        type = HIF_RX_LOG;
    }
    else {

        #ifndef SDIO_DEAGGR
        if (aggr_num) {
            skb_queue_tail(&asr_hw->rx_to_os_skb_list, skb);
            uwifi_msg_rx_to_os_queue();
        } else {
            // deagg sdio buf done, free skb_head here.
            memset(skb->data, 0, asr_hw->ipc_env->rx_bufsz);
            // Add the sk buffer structure in the table of rx buffer
            skb_queue_tail(&asr_hw->rx_data_sk_list, skb);
        }
        #else

        // start_port > 1, de-agg sdio multi-port read here.
        // when enable ooo, sdio buf contain data or rxdesc.
                // like ipc_host_process_rx_usb , process ooo here.
        asr_sdio_deaggr(asr_hw,skb,aggr_num);
        #endif
        type = HIF_RX_DATA;
    }

    return type;
}

#if 0
int sss0_pre = 0;
int sss0 = 0;
int sss1 = 0;
int sss2 = 0;
int sss3 = 0;
int sss4 = 0;
int sss5 = 0;
#endif

uint8_t get_sdio_reg(uint8_t idx)
{
    uint8_t *p_reg;
    struct asr_hw *asr_hw;

    asr_hw = uwifi_get_asr_hw();

    p_reg = asr_hw->sdio_reg + idx;

    return *p_reg;
}


static int host_int_tx(struct asr_hw *asr_hw, u8 * regs)
{
    u16 wr_bitmap, diff_bitmap;

    //read tx available port
    wr_bitmap = regs[WR_BITMAP_L];
    wr_bitmap |= regs[WR_BITMAP_U] << 8;

    //update tx available port
#ifdef TXBM_DELAY_UPDATE
    //reserve 1 bit to delay updating
    wr_bitmap &= ~(asr_hw->tx_last_trans_bitmap);
#endif
    asr_rtos_lock_mutex(&asr_hw->tx_msg_lock);
    diff_bitmap = ((asr_hw->tx_use_bitmap) ^ wr_bitmap);
    asr_hw->tx_use_bitmap = wr_bitmap;
    asr_rtos_unlock_mutex(&asr_hw->tx_msg_lock);

    if (txlogen) {
        dbg(D_ERR, D_UWIFI_CTRL, "[%d]wbm 0x%x 0x%x, %d\n", 0, wr_bitmap, diff_bitmap, asr_hw->tx_data_cur_idx);
    }

    //if (asr_xmit_opt == false)
    //    tx_aggr_stat(&(asr_hw->tx_agg_env), diff_bitmap);

    //if (diff_bitmap && (bitcount(diff_bitmap) <= 16)) {
    //    tx_bitmap_change_cnt[bitcount(diff_bitmap) - 1]++;
    //}

    #if 0
    if (asr_hw->last_sdio_regs[WR_BITMAP_L] != regs[WR_BITMAP_L]
        || asr_hw->last_sdio_regs[WR_BITMAP_U] != regs[WR_BITMAP_U]) {

        asr_hw->sdio_reg[WR_BITMAP_L] = regs[WR_BITMAP_L];
        asr_hw->sdio_reg[WR_BITMAP_U] = regs[WR_BITMAP_U];
        asr_hw->last_sdio_regs[WR_BITMAP_L] = regs[WR_BITMAP_L];
        asr_hw->last_sdio_regs[WR_BITMAP_U] = regs[WR_BITMAP_U];
    }
    #endif

    // /ipc_write_sdio_data_cnt++;
    return 0;
}


static int port_dispatch(struct asr_hw *asr_hw, u16 port_size,
             u8 start_port, u8 end_port, u8 io_addr_bmp, u8 aggr_num, const u8 asr_sdio_reg[])
{
#define ASR_IOPORT(hw) ((hw)->ioport & (0xffff0000))
    //u8 reg_val;
    int type;
    int ret = 0;
    u32 addr;
    struct sk_buff *skb = NULL;
    //struct sdio_func *func = asr_hw->plat->func;
    //int seconds;

    if (start_port == end_port) {
        port_size = asr_sdio_reg[RD_LEN_L(start_port)];
        port_size |= asr_sdio_reg[RD_LEN_U(start_port)] << 8;
        if (port_size == 0) {
            dbg(D_ERR, D_UWIFI_CTRL, "%s read bitmap port %d value:0x%x\n", __func__, start_port, port_size);
            return -ENODEV;
        }
        //dbg(D_ERR, D_UWIFI_CTRL, "!!!!len2 %d\n",port_size);

        addr = ASR_IOPORT(asr_hw) | start_port;
    } else {        //aggregation mode
        addr = ASR_IOPORT(asr_hw) | 0x1000 | start_port | (io_addr_bmp << 4);
    }

    if (start_port == SDIO_PORT_IDX(0))
        skb = skb_dequeue(&asr_hw->rx_msg_sk_list);
    else
        skb = skb_dequeue(&asr_hw->rx_data_sk_list);

    if (!skb) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s no more skb buffer\n", __func__);
        return -ENOMEM;
    }
    //seconds = ktime_to_us(ktime_get());
    //dbg("rdb %d\n%s",seconds, "\0");


    if (((port_size > IPC_RXBUF_SIZE) && (start_port != SDIO_PORT_IDX(0))) ||
        ((port_size > IPC_RXMSGBUF_SIZE) && (start_port == SDIO_PORT_IDX(0)))) {
        dbg(D_ERR, D_UWIFI_CTRL, "[%s][%d %d]!!!!len2 %d\n", __func__, start_port, end_port, port_size);
    }


    asr_sdio_claim_host();
    ret = asr_sdio_read_cmd53(addr, skb->data, port_size);
    asr_sdio_release_host();


    //sss5 = ktime_to_us(ktime_get());

    //seconds = ktime_to_us(ktime_get());
    //dbg("rda %d\n%s",((int)ktime_to_us(ktime_get())-seconds), "\0");

    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s read data failed %x %x\n", __func__, addr, port_size);


        if (start_port == SDIO_PORT_IDX(0)) {
            memset(skb->data, 0, IPC_RXMSGBUF_SIZE);
            skb_queue_tail(&asr_hw->rx_msg_sk_list, skb);
        } else {
            memset(skb->data, 0, asr_hw->ipc_env->rx_bufsz);
            skb_queue_tail(&asr_hw->rx_data_sk_list, skb);
        }

        return ret;
    }


    #ifndef SDIO_DEAGGR
    if (start_port > 1) {
        ((struct host_rx_desc *)skb->data)->num = aggr_num;
    }

    //had read data,call back function
    type = ipc_host_process_rx_sdio(start_port, asr_hw, skb,aggr_num);
    #else

    // start_port > 1, de-agg sdio multi-port read here.
    // add new list for multi-port.   data or rxdesc.

    type = ipc_host_process_rx_sdio(start_port, asr_hw, skb,aggr_num);

    #endif

    //if (type == HIF_RX_DATA)
    //    asr_hw->stats.last_rx = jiffies;

    return 0;
}


int port_dispatch_nomem_retry(struct asr_hw *asr_hw,uint16_t port_size,
                                uint8_t start_port,uint8_t end_port, uint8_t io_addr_bmp,uint8_t aggr_num, const u8 regs[])
{
    int wait_mem_cnt = 0;
    int rx_skb_free_cnt, rx_skb_to_os_cnt;

    do {
        int ret = port_dispatch(asr_hw,port_size,start_port,end_port,io_addr_bmp,aggr_num,regs);

        if (ret != -ENOMEM)    // if not enomem error, use host_restart recovery.
            return ret;

        asr_rtos_delay_milliseconds(1);    // wait 20ms
        rx_skb_free_cnt = skb_queue_len(&asr_hw->rx_data_sk_list);
        rx_skb_to_os_cnt = skb_queue_len(&asr_hw->rx_to_os_skb_list);

        dbg(D_ERR, D_UWIFI_CTRL,
            "lalala rx fail ENOMEM,wait & re-read: %d %d %d,free skb_cnt=%d,to_os_cnt=%d\n",
            ret, wait_mem_cnt, start_port, rx_skb_free_cnt, rx_skb_to_os_cnt);

    } while (wait_mem_cnt++ < 100);

    dbg(D_ERR, D_UWIFI_CTRL, "unlikely happen, nomem retry fail!!!");

    return -ENOMEM;
}


const int nx_txdesc_cnt[] =
{
    NX_TXDESC_CNT0,
    NX_TXDESC_CNT1,
    NX_TXDESC_CNT2,
    NX_TXDESC_CNT3,
    #if NX_TXQ_CNT == 5
    NX_TXDESC_CNT4,
    #endif
};

const int nx_txdesc_cnt_msk[] =
{
    NX_TXDESC_CNT0 - 1,
    NX_TXDESC_CNT1 - 1,
    NX_TXDESC_CNT2 - 1,
    NX_TXDESC_CNT3 - 1,
    #if NX_TXQ_CNT == 5
    NX_TXDESC_CNT4 - 1,
    #endif
};

uint8_t asr_get_rx_aggr_end_port(struct asr_hw *asr_hw,uint8_t start_port, uint16_t *hw_bitmap, uint8_t *io_addr_bitmap, uint8_t *num, uint16_t *len)
{
    uint8_t end_port = start_port;
    uint16_t port_size;

    *io_addr_bitmap = 0;
    *num = 0;
    *len = 0;
    while(*hw_bitmap)
    {
        if(*hw_bitmap & (1<<end_port))
        {
            // first check port size
            port_size = get_sdio_reg(RD_LEN_L(end_port));
            port_size |= get_sdio_reg(RD_LEN_U(end_port)) << 8;
            if (port_size == 0) {
                dbg(D_ERR, D_UWIFI_CTRL, "%s read bitmap port %d value:0x%x\n", __func__, end_port, port_size);
            }
            //if(lalalaen)
            //    pr_err("len %d %d %d",port_size,asr_hw->sdio_reg[RD_LEN_L(end_port)],asr_hw->sdio_reg[RD_LEN_U(end_port)]);

            // second check max len.
            if ((*len + port_size) > IPC_RXBUF_SIZE) {
                //dbg(D_ERR, D_UWIFI_CTRL, "len=%d oversize,max=%d ,%d %d",*len,IPC_RXBUF_SIZE,start_port,end_port);

                if (end_port == 2)
                    end_port = 15;
                else
                    end_port--;

                return end_port;
            }

            // now we can use this port.
            *hw_bitmap &= ~(1<<end_port);

            *len += port_size;

            if(end_port < start_port)
            {
                *io_addr_bitmap |= (1 << (*num+2));
                (*num)++;
                if(*num >= 6 || *num >= RX_AGGR_BUF_NUM)
                    return end_port;
                end_port++;
            }
            else
            {
                *io_addr_bitmap |= (1 << *num);
                (*num)++;
                if(*num >= RX_AGGR_BUF_NUM)//the maximum rx aggr num
                    return end_port;
                if(++end_port == 16)
                {
                    if(*num >= 6 || *num >= RX_AGGR_BUF_NUM)
                    {
                        end_port--;
                        return end_port;
                    }
                    else
                        end_port = 2;
                }
            }
        }
        else
        {
            dbg(D_ERR, D_UWIFI_CTRL, "lalala cur_port_idx %d no data, have %d data, rx data idx:%d\n",end_port, *num, asr_hw->rx_data_cur_idx);
            break;
        }
    }

    if(end_port == 2)
        end_port = 15;
    else
        end_port--;

    return end_port;
}


bool host_restart(struct asr_hw *asr_hw, u8 * regs, u8 aggr_num, bool is_sdiotx)
{
    u8 fw_reg_idx = 0, fw_reg_num = 0;
    int wait_fw_cnt = 0;
    bool re_read_done_flag = false;

    dbg(D_ERR, D_UWIFI_CTRL,"H2C_INTEVENT(0x0):0x4 firstly terminate the cmd53! (%d %d %d) \n",
            asr_hw->tx_data_cur_idx, asr_hw->rx_data_cur_idx,aggr_num);

    ///step 1 : host set aggr parameter and finish cmd53.
    #define ASR_SDIO_RESET_TX   (1<<7)

    asr_sdio_claim_host();
    if (is_sdiotx) {
        sdio_writeb_cmd52(SCRATCH1_0,asr_hw->tx_data_cur_idx | ASR_SDIO_RESET_TX);
    } else {
        sdio_writeb_cmd52(SCRATCH1_0,asr_hw->rx_data_cur_idx);
    }
    sdio_writeb_cmd52(SCRATCH1_1,aggr_num);
    // clear fw notify reg.
    sdio_writeb_cmd52(SCRATCH1_2,0);
    sdio_writeb_cmd52(SCRATCH1_3,0);

    // 0x0(host to card event,bit2(0x4) means CMD53_fin_host,current cmd53 data transfer will terminate.)
    sdio_writeb_cmd52(H2C_INTEVENT,0x4);
    asr_sdio_release_host();

    ///step2 : check read recovery status.
    // if abort event trigger fail, use H2C event trigger.
    asr_sdio_claim_host();
    // 0x0(host to card event,bit3(0x8) means software event from driver to card.)
    sdio_writeb_cmd52(H2C_INTEVENT,0x8);   // trigger host to card event for read recovery.
    asr_sdio_release_host();

    //wait for fw ready for this idx
    do {
        asr_rtos_delay_milliseconds(20);
        wait_fw_cnt++;

        asr_sdio_claim_host();
        sdio_readb_cmd52(SCRATCH1_2, &fw_reg_idx);
        sdio_readb_cmd52(SCRATCH1_3, &fw_reg_num);

        asr_sdio_release_host();

        dbg(D_ERR, D_UWIFI_CTRL,"H2C host restart check %d:%d %d \n", wait_fw_cnt, fw_reg_idx, fw_reg_num);

        if (fw_reg_idx > 0 && fw_reg_idx < 16) {
            re_read_done_flag = true;

            if (is_sdiotx) {
                asr_hw->tx_data_cur_idx = fw_reg_idx;

                asr_hw->tx_use_bitmap = 0;

            } else {
                if (fw_reg_idx != 1) {
                    //todo fix
                    asr_hw->rx_data_cur_idx = fw_reg_idx;
                }
            }
            break;
        }

    } while (wait_fw_cnt < 10);

    dbg(D_ERR, D_UWIFI_CTRL,"HOST_RESTART:ret=%d,is_sdiotx=%d,fw_reg_idx=%u,fw_reg_num=%u\n"
        ,re_read_done_flag,is_sdiotx,fw_reg_idx,fw_reg_num);

    asr_rtos_delay_milliseconds(10);

    return re_read_done_flag;
}

void rx_bitmap_nodata_clear(struct asr_hw *asr_hw, uint8_t start_port, u16 bitmap, u8 * regs)
{
    uint8_t end_port,io_addr_bmp=0;
    uint8_t aggr_num = 0;
    uint16_t len = 0;

    while(bitmap & 0xFFFC) {
        while(!(bitmap & (1 << start_port))) {
            if (++start_port > 15) {
                start_port = 2;
            }
        }

        end_port = asr_get_rx_aggr_end_port(asr_hw, start_port, &bitmap, &io_addr_bmp, &aggr_num, &len);

        dbg(D_ERR, D_UWIFI_CTRL, "rx clear:%u,%u,%u,%u,0X%X,0X%X\n",start_port,end_port,aggr_num,len,io_addr_bmp,bitmap);

        aggr_num = 0;
        port_dispatch_nomem_retry(asr_hw,len,start_port,end_port,io_addr_bmp,aggr_num,regs);

    }

}

static u16 rx_aggregate(struct asr_hw *asr_hw, u16 bitmap,u8 * regs)
{
    uint8_t start_port, end_port,io_addr_bmp=0;
    uint8_t aggr_num = 0;
    uint16_t len = 0;
    u16 rd_bitmap_ori = get_sdio_reg(RD_BITMAP_L) | (get_sdio_reg(RD_BITMAP_U) << 8) ;
    int ret;

    start_port = asr_hw->rx_data_cur_idx;

    if (!(bitmap & (1<<start_port)))
    {
        dbg(D_ERR, D_UWIFI_CTRL,"lalala cur_rx_idx %d no data , cur_bitmap:0x%x , ori_bitmap:0x%x \n",start_port,bitmap,rd_bitmap_ori);

        rx_bitmap_nodata_clear(asr_hw, start_port, bitmap, regs);

        return 0;
    }

    end_port = asr_get_rx_aggr_end_port(asr_hw,start_port, &bitmap, &io_addr_bmp, &aggr_num, &len);
    //dbg(D_ERR, D_UWIFI_CTRL, "[%s] recv start:%d end:%d, aggr num:%d len:%d rx_data_cur_idx:%d bitmap:0x%x ori_bitmap:0x%x\r\n", __func__,
    //                                         start_port, end_port, aggr_num, len, asr_hw->rx_data_cur_idx, bitmap, temp_bitmap);

    #ifdef CONFIG_ASR_KEY_DBG
    rx_aggr_stat(aggr_num);
    #endif

    if((ret = port_dispatch_nomem_retry(asr_hw,len,start_port,end_port,io_addr_bmp,aggr_num,regs))) {
        dbg(D_ERR, D_UWIFI_CTRL,"lalala rx DATA err %d - 0x%x - cur-idx:%d\n",ret, rd_bitmap_ori, asr_hw->rx_data_cur_idx);

        if (asr_test_bit(ASR_DEV_RESTARTING, &asr_hw->phy_flags)) {

            dbg(D_ERR, D_UWIFI_CTRL,"%s: break drv_flags=0X%X\n", __func__, (unsigned int)asr_hw->phy_flags);

        } else if (host_restart(asr_hw, regs, aggr_num, false)) {   // other rx err recovery.

            dbg(D_ERR, D_UWIFI_CTRL, "HOST_RESTART: SDIO read SUCCESS !!!!! \n");
            return 0;
        } else {
            dbg(D_ERR, D_UWIFI_CTRL, "HOST_RESTART: SDIO read FAIL !!!!! \n");
        }
    } else {
        asr_rx_pkt_cnt += aggr_num;
    }

    end_port++;
    if(end_port >= 16)
        start_port = 2;
    else
        start_port = end_port;
    asr_hw->rx_data_cur_idx = start_port;

    return bitmap;

}

int asr_wait_rx_sk_buf_cnt;
int asr_wait_rx_sk_msgbuf_cnt;

int asr_wait_rx_data_sk_buf(struct asr_hw *asr_hw)
{
    int wait_mem_cnt = 0;
    int ret = 0;

    asr_wait_rx_sk_buf_cnt++;

    do {
        wait_mem_cnt++;
        asr_rtos_delay_milliseconds(1);   // wait 20ms

    } while ((skb_queue_empty(&asr_hw->rx_data_sk_list))
         && (wait_mem_cnt < 100));

    if (skb_queue_empty(&asr_hw->rx_data_sk_list)) {
        ret = -1;
        dbg(D_ERR, D_UWIFI_CTRL, "%s ,ret=%d,%d,%d\n", __func__,ret,skb_queue_len(&asr_hw->rx_data_sk_list),skb_queue_len(&asr_hw->rx_to_os_skb_list));
    }

    return ret;
}

int asr_wait_rx_msg_sk_buf(struct asr_hw *asr_hw)
{
    int wait_mem_cnt = 0;
    int ret = 0;

    asr_wait_rx_sk_msgbuf_cnt++;

    do {
        wait_mem_cnt++;
        asr_rtos_delay_milliseconds(5);   // wait 20ms

    } while ((skb_queue_empty(&asr_hw->rx_msg_sk_list))
         && (wait_mem_cnt < 100));

    if (skb_queue_empty(&asr_hw->rx_msg_sk_list))
        ret = -1;

    dbg(D_ERR, D_UWIFI_CTRL, "%s ,ret=%d \n", __func__,ret);

    return ret;
}

int ipc_host_msg_push(struct ipc_host_env_tag *env, void *msg_buf, uint16_t len)
{
    struct asr_hw *asr_hw = (struct asr_hw *)env->pthis;
    uint8_t *src = (uint8_t*)((struct asr_cmd *)msg_buf)->a2e_msg;
    uint8_t type = HIF_TX_MSG;


    return asr_sdio_send_data(asr_hw, type, src, len, (asr_hw->ioport | 0x0), 0x1);
}


// return reschedule
int asr_total_int_rx_cnt;
static int host_int_rx(struct asr_hw *asr_hw, u8 * regs)
{
    u16 rd_bitmap;
    u16 rd_bitmap_new = 0;
    int rx_skb_free_cnt;
    int ret;
	uint16_t times = 0;

    rd_bitmap = regs[RD_BITMAP_L];
    rd_bitmap |= regs[RD_BITMAP_U] << 8;

    if (rxlogen)
        dbg(D_ERR, D_UWIFI_CTRL, "rbm 0x%x\n", rd_bitmap);

    if (rd_bitmap == 0)
        return 0;


    asr_total_int_rx_cnt++;

    if (skb_queue_empty(&asr_hw->rx_msg_sk_list)) {
        if (asr_wait_rx_msg_sk_buf(asr_hw) < 0) {
            return 1;
        }
    }


    //msg
    if (rd_bitmap & (1 << SDIO_PORT_IDX(0))) {
        if ((ret = port_dispatch(asr_hw, 0, 0, 0, 0, 0, regs)))
            dbg(D_ERR, D_UWIFI_CTRL, "lalala rx MSG err %d\n", ret);
        else {
            asr_rx_pkt_cnt++;
        }
        rd_bitmap &= 0xFFFE;
    }


    if (skb_queue_empty(&asr_hw->rx_data_sk_list)) {
        if (asr_wait_rx_data_sk_buf(asr_hw) < 0) {
            return 1;
        }
    }

    //log
    if (rd_bitmap & (1 << SDIO_PORT_IDX(1))) {
        if ((ret = port_dispatch(asr_hw, 0, 1, 1, 0, 0, regs)))
            dbg(D_ERR, D_UWIFI_CTRL, "lalala rx LOG err %d\n", ret);
        else {
            asr_rx_pkt_cnt++;
        }
        rd_bitmap &= 0xFFFD;
    }

    if (rd_bitmap == 0) {
        //dbg(D_ERR, D_UWIFI_CTRL, "[%s %d]get rd_bitmap failed\n", __func__,__LINE__);
        return 0;
    }
    //data port start
    //mutex_lock(&asr_process_int_mutex);
    if (!(rd_bitmap & (1 << asr_hw->rx_data_cur_idx))) {

        //mutex_unlock(&asr_process_int_mutex);

        dbg(D_ERR, D_UWIFI_CTRL, "%s:idx %d no data(0x%x,0x%x),%d,%d\n ",
            __func__, asr_hw->rx_data_cur_idx, rd_bitmap, rd_bitmap_new, ret, asr_hw->restart_flag);

        rx_bitmap_nodata_clear(asr_hw, asr_hw->rx_data_cur_idx, rd_bitmap, regs);

        return 0;
    }

    //ipc_read_sdio_data_cnt++;

    //data,like (0000 0000,0111,1100)
    while (rd_bitmap && times++ < 10) {
        if (skb_queue_empty(&asr_hw->rx_data_sk_list)) {
            if (asr_wait_rx_data_sk_buf(asr_hw) < 0) {
               // mutex_unlock(&asr_process_int_mutex);
                return 1;
            }
        }

        if (asr_test_bit(ASR_DEV_RESTARTING, &asr_hw->phy_flags)) {

            dbg(D_ERR, D_UWIFI_CTRL, "%s: break phy_flags=0X%lX\n", __func__, asr_hw->phy_flags);
            break;
        }

        rd_bitmap = rx_aggregate(asr_hw, rd_bitmap, regs);

    }
    //mutex_unlock(&asr_process_int_mutex);

    return 0;
}

int asr_process_int_status(struct ipc_host_env_tag *env)
{
    int ret = 0;
    u8 sdio_ireg;
    u8 sdio_regs[68] = { 0 };
    struct asr_hw *asr_hw = (struct asr_hw *)env->pthis;

    #if 0
    if (asr_hw->restart_flag) {
        //spin_lock_bh(&asr_hw->pmain_proc_lock);
        asr_hw->restart_flag = false;
        //spin_unlock_bh(&asr_hw->pmain_proc_lock);
        ret = read_sdio_block_retry(asr_hw, sdio_regs, &sdio_ireg);
        if (ret) {
            dbg(D_ERR, D_UWIFI_CTRL, "%s:read sdio_reg fail %d!!! \n", __func__, ret);
            return -1;
        }
    } else
    #endif
    {
        // get int reg and clean.
        //spin_lock_bh(&asr_hw->int_reg_lock);
        sdio_ireg = asr_hw->sdio_ireg;
        asr_hw->sdio_ireg = 0;
        memcpy(sdio_regs, asr_hw->last_sdio_regs, SDIO_REG_READ_LENGTH);
        //spin_unlock_bh(&asr_hw->int_reg_lock);
    }


    if (sdio_ireg & HOST_INT_UPLD_ST) {
        ret = host_int_rx(asr_hw, sdio_regs);
    }

    if (sdio_ireg & HOST_INT_DNLD_ST) {
        ret = host_int_tx(asr_hw, sdio_regs);
    }

    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "unlikely:lalala rx fail ENOMEM, %d\n", ret);
        //set_bit(ASR_FLAG_MAIN_TASK_BIT,&asr_hw->ulFlag);
        //wake_up_interruptible(&asr_hw->waitq_rx_thead);
    }

    return 0;
}
