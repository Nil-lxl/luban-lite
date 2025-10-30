#include <stdio.h>
#include "uwifi_sdio.h"
#include "asr_rtos_api.h"
#include "asr_dbg.h"
#include "uwifi_hif.h"
#include "uwifi_msg.h"

#if !defined ALIOS_SUPPORT && !defined JL_SDK && !defined THREADX_STM32 && !defined AWORKS && !defined STM32H7_SDK && !defined(HX_SDK) && !defined(JXC_SDK)
#include "gpio.h"
#include "sdio.h"
#else
#include "asr_gpio.h"
#include "asr_sdio.h"
#endif

#ifdef THREADX
#include "timer.h"
#include "cgpio.h"
#include "sdio_api.h"
#endif


//extern int tx_conserve;

asr_hisr_t sdio_card_gpio_hisr = NULL;
bool fw_download_done = 0;
extern uint8_t is_fw_downloaded;
extern bool txlogen;
int count_sdio_card_gpio_evt = 0;


int tx_aggr = 8;


void sdio_card_gpio_hisr_func(void)
{
    //dbg(D_ERR, D_UWIFI_CTRL, "%s ..\r\n", __func__);

    uwifi_sdio_event_set(UWIFI_SDIO_EVENT_RX);

    if (txlogen)
        dbg(D_ERR, D_UWIFI_CTRL, "tri rx ..\r\n");

    count_sdio_card_gpio_evt ++;
}

/*
void sdio_card_gpio_isr(void)
{
    if (0 == fw_download_done || NULL == sdio_card_gpio_hisr|| 0 == is_fw_downloaded)//ignore it until firmware done
        return;
    // dbg(D_ERR, D_UWIFI_CTRL, "%s ...\r\n", __func__);
    //count_sdio_card_gpio_isr ++;
    asr_rtos_active_hisr(&sdio_card_gpio_hisr);
}

GPIOConfiguration sdio_card_gpio = {
    GPIO_IN_PIN,
    0,
    GPIO_PULLUP_ENABLE,
    GPIO_TWO_EDGE,
    sdio_card_gpio_isr,
    NULL,
};
*/
/*
get_ioport: get ioport value
*/
int sdio_get_ioport(void)
{
    bool ret;
    uint8_t reg;
    int ioport;
    dbg(D_DBG, D_UWIFI_CTRL, "%s start...\r\n", __func__);
    /*get out ioport value*/
    asr_sdio_claim_host();
    ret = sdio_readb_cmd52(IO_PORT_0, &reg);
    asr_sdio_release_host();
    if (!ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s can't read 0x%x\n", __func__, IO_PORT_0);
        goto exit;
    }

    ioport = reg & 0xff;
    asr_sdio_claim_host();
    ret = sdio_readb_cmd52(IO_PORT_1, &reg);
    asr_sdio_release_host();
    if (!ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s can't read 0x%x\n", __func__, IO_PORT_1);
        goto exit;
    }


    ioport |= ((reg & 0xff) << 8);
    asr_sdio_claim_host();
    ret = sdio_readb_cmd52(IO_PORT_2, &reg);
    asr_sdio_release_host();
    if (!ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s can't read 0x%x\n", __func__, IO_PORT_2);
        goto exit;
    }
    ioport |= ((reg & 0xff) << 16);
    dbg(D_DBG, D_UWIFI_CTRL, "%s end ioport:0x%X\r\n", __func__, ioport);
    return ioport;
exit:
    return -1;
}

int asr_sdio_config_rsr(uint8_t mask)
{
    uint8_t reg_val;
    bool ret = 0;

    asr_sdio_claim_host();
    ret = sdio_readb_cmd52(HOST_INT_RSR, &reg_val);
    asr_sdio_release_host();
    if (!ret)
        return ret;

    reg_val |= mask;

    asr_sdio_claim_host();
    ret = sdio_writeb_cmd52(HOST_INT_RSR, reg_val);
    asr_sdio_release_host();

    return ret;
}

//extern int scan_done_flag;
unsigned char asr_sdio_read_cmd53(unsigned int dataport, unsigned char *data, int size)
{
    //int retry_cnt = 0;
    int ret = -1;
    //while(retry_cnt++<300 &&  SCAN_DEINIT != scan_done_flag)
    //{

        ret = sdio_read_cmd53(dataport, data, size);

        #if 0
        if(ret)
        {
            dbg(D_ERR, D_UWIFI_CTRL,"%s err ret:%d cnt:%d, addr:0x%x,len:%d\n",__func__,ret,retry_cnt,dataport,size);
            #if 1
            // if read cmd53 error need funnum0 0ffset06 set bit0-bit2==1
            int _ret = SDIO_CMD52(1,0,0x06,1,0);    // write func0 0x6(I/O Abort reg) , 0x1 means to stop func1 transfer.
            dbg(D_ERR, D_UWIFI_CTRL,"%s fix ret=%d\n",__func__,_ret);
            asr_rtos_delay_milliseconds(25);// sleep more time
            _ret = SDIO_CMD52(1,1,0x03,0,0);        // write func1 0x3(host interrupt status)  , clear host interrupt status
            dbg(D_ERR, D_UWIFI_CTRL,"%s host_intstatus ret=%d\n",__func__,_ret);
            asr_rtos_delay_milliseconds(20);// sleep more time
            #endif
            continue;
        }
        else
            break;
        #endif
    //}

    return (unsigned char)ret;
}

int asr_sdio_config_auto_reenable(void)
{
    uint8_t reg_val;
    bool ret = 0;

    asr_sdio_claim_host();
    ret = sdio_readb_cmd52(SDIO_CONFIG2, &reg_val);
    asr_sdio_release_host();
    if (!ret)
        return ret;

    reg_val |= AUTO_RE_ENABLE_INT;

    asr_sdio_claim_host();
    ret = sdio_writeb_cmd52(SDIO_CONFIG2, reg_val);
    asr_sdio_release_host();

    return ret;
}

int asr_sdio_init_config(struct asr_hw *asr_hw)
{
    int ioport;
    int ret = 0;

    /*step 0: get ioport*/
    ioport = sdio_get_ioport();
    if (ioport < 0) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s get ioport failed\n", __func__);
        goto exit;
    }
    asr_hw->ioport = ioport & 0xffff0000;

    /*step 1: config host_int_status(0x03) clear when read*/
    ret = asr_sdio_config_rsr(HOST_INT_RSR_MASK);
    if (!ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s config int rsr failed\n", __func__);
        goto exit;
    }

    /*step 2: config auto re-enable(0x6c) bit4*/
    ret = asr_sdio_config_auto_reenable();
    if (!ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s config auto re-enable failed\n", __func__);
        goto exit;
    }
    return 0;
exit:
    return -1;
}

uint8_t asr_get_tx_aggr_bitmap_addr(uint8_t start_port, uint8_t end_port)
{
    int i;
    uint8_t bitmap = 0;
    if(start_port <= end_port)
    {
        for(i=0; i<end_port-start_port+1; i++)
        {
            bitmap |= (1<<i);
        }
    }
    else
    {
        for(i=0; i<end_port+17-start_port; i++)
        {
            if(start_port+i != 16)
                bitmap |= (1<<i);
        }
    }
    return bitmap;
}

uint16_t asr_get_tx_bitmap_from_ports(uint8_t start_port, uint8_t end_port)
{
    int i;
    uint16_t bitmap = 0;

    if(start_port <= end_port)
    {
        for(i=start_port; i<=end_port; i++)
        {
            bitmap |= (1<<i);
        }
    }
    else
    {
        for(i=start_port; i<=15; i++)
        {
            bitmap |= (1<<i);
        }
        for(i=1; i<=end_port; i++)
        {
            bitmap |= (1<<i);
        }
    }
    return bitmap;
}


int asr_sdio_tx_common_port_dispatch(struct asr_hw *asr_hw, u8 * src, u32 len, unsigned int io_addr, u16 bitmap_record)
{
    int ret = 0;
    //struct sdio_func *func = asr_hw->plat->func;

    asr_sdio_claim_host();
    ret = sdio_write_cmd53(io_addr, src, len);

    asr_rtos_lock_mutex(&asr_hw->tx_msg_lock);
    asr_hw->tx_use_bitmap &= ~(bitmap_record);
    asr_rtos_unlock_mutex(&asr_hw->tx_msg_lock);
#ifdef TXBM_DELAY_UPDATE
    asr_hw->tx_last_trans_bitmap = bitmap_record;
#endif

    asr_sdio_release_host();

    //seconds = ktime_to_us(ktime_get());
    //dbg("tda %d\n%s",((int)ktime_to_us(ktime_get())-seconds), "\0");

    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s write data failed,ret=%d\n", __func__, ret);
    }

    return ret;
}


int asr_sdio_enable_int(uint8_t mask)
{
    uint8_t reg_val, reg_val1;
    int ret = -1;

    if (!(mask & HOST_INT_MASK)) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s 0x%x is invalid int mask\n", __func__, mask);
        return ret;
    }

    /*enable specific interrupt*/
    asr_sdio_claim_host();
    ret = sdio_readb_cmd52(HOST_INTMASK, &reg_val);
    asr_sdio_release_host();
    if (!ret)
        return ret;

    reg_val |= (mask & HOST_INT_MASK);
    asr_sdio_claim_host();
    ret = sdio_writeb_cmd52(HOST_INTMASK, reg_val);
    ret = sdio_readb_cmd52(HOST_INTMASK, &reg_val1);
    asr_sdio_release_host();
    if (!ret)
        return ret;

    return ret;
}

int asr_sdio_disable_int(uint8_t mask)
{
    uint8_t reg_val;
    int ret = -1;

    /*enable specific interrupt*/
    asr_sdio_claim_host();
    ret = sdio_readb_cmd52(HOST_INTMASK, &reg_val);
    asr_sdio_release_host();
    if (!ret)
    {
        dbg(D_ERR, D_UWIFI_CTRL,"%s read host int mask failed\n", __func__);
        return ret;
    }

    reg_val &= ~mask;
    asr_sdio_claim_host();
    ret = sdio_writeb_cmd52(HOST_INTMASK, reg_val);
    asr_sdio_release_host();
    if (!ret)
    {
        dbg(D_ERR, D_UWIFI_CTRL,"%s disable host int mask failed\n", __func__);
        return ret;
    }

    asr_sdio_claim_host();
    ret = sdio_readb_cmd52(HOST_INTMASK, &reg_val);
    asr_sdio_release_host();
    if (!ret)
    {
        dbg(D_ERR, D_UWIFI_CTRL,"%s read host int mask failed\n", __func__);
        return ret;
    }
    return ret;
}

int asr_sdio_detect_change()
{
    return sdio_bus_probe();
}

uint32_t crc32_recursive(const uint8_t *buf, uint32_t len, uint32_t crc_in)
{
    uint32_t i;
    uint32_t crc = ~crc_in;

    while (len--) {
        crc ^= *buf++;
        for (i = 0; i < 8; ++i) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc = (crc >> 1);
        }
    }
    return ~crc;
}

#if 0
static void sdio_delay(uint32_t count)
{
    uint32_t i = 0;
    uint32_t j = 0;
    for(i=0; i<count; i++)
    {
        for(j=0; j<30000; j++)
        {
            ;
        }
    }
}
#endif

/*
    poll_card_status: check if card in ready state
*/
int poll_card_status(uint8_t mask)
{
    uint32_t tries;
    uint8_t reg_val;
    int ret;

    for (tries = 0; tries < MAX_POLL_TRIES; tries++) {
        asr_sdio_claim_host();
        ret = sdio_readb_cmd52(C2H_INTEVENT, &reg_val);
        asr_sdio_release_host();
        if (!ret) {
            dbg(D_ERR, D_UWIFI_CTRL, "%s can't read 0x%x out\n", __func__, C2H_INTEVENT);
            break;
        }
        //dbg(D_ERR, D_UWIFI_CTRL, "%s reg_val:0x%x mask:0x%x\n", __func__, reg_val, mask);
        if ((reg_val & mask) == mask)
            return 0;
        else {
            dbg(D_ERR, D_UWIFI_CTRL, "%d regval 0x%x\n", tries,reg_val);
        }

        //sdio_delay(10);
        asr_rtos_delay_milliseconds(10);
    }

    dbg(D_ERR, D_UWIFI_CTRL, "[poll_card_status]fail:%d regval 0x%x\n", tries,reg_val);

    return -1;
}

/*
    check_scratch_status: check scratch register value
*/
int check_scratch_status(uint16_t status)
{
    uint32_t tries;
    //uint16_t scratch_val;
    uint8_t scratch_val;
    int ret;


    for (tries = 0; tries < MAX_POLL_TRIES * 10; tries++) {
        asr_sdio_claim_host();
        ret = sdio_readb_cmd52(SCRATCH_1, &scratch_val);
        asr_sdio_release_host();
        if (!ret) {
            dbg(D_ERR, D_UWIFI_CTRL, "can't read 0x%x out\n",  SCRATCH_1);
            break;
        }
        if ((scratch_val << 8) & status) {
            dbg(D_ERR, D_UWIFI_CTRL, "scratch_val1:0x%x status:0x%x\n", scratch_val, status);
            return 0;  //success
        } else {
            dbg(D_WRN, D_UWIFI_CTRL, "%d regval 0x%x\n",tries,scratch_val);
        }

        //sdio_delay(1);
        asr_rtos_delay_milliseconds(20);
    }

    dbg(D_ERR, D_UWIFI_CTRL, "check scratch fail:%d regval 0x%x\n",tries,scratch_val);
    asr_sdio_claim_host();
    ret = sdio_readb_cmd52(SCRATCH_0, &scratch_val);
    asr_sdio_release_host();
    if (!ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "can't read 0x%x out\n",  SCRATCH_0);
    } else {
        dbg(D_ERR, D_UWIFI_CTRL, "check scratch0 :regval 0x%x\n",scratch_val);
    }

    return -1;//fail
}

//extern int tx_aggr;
#define MAX_AGG_THRESHOLD 1

uint8_t asr_sdio_tx_get_available_data_port(struct asr_hw *asr_hw,
                       uint16_t ava_pkt_num, uint8_t * port_num, unsigned int *io_addr, uint16_t * bitmap_record)
{
    u16 bitmap;
    u8 start_port;
    u8 end_port;
    uint8_t port_idx;
    //u16 bitmap_record = 0x0;
    *bitmap_record = 0;
#if 1
    if (ava_pkt_num > tx_aggr)
        ava_pkt_num = tx_aggr;
    *io_addr = 0x0;
    port_idx = 0x0;
    bitmap = asr_hw->tx_use_bitmap;

    if (bitmap & (1 << asr_hw->tx_data_cur_idx)) {
        start_port = asr_hw->tx_data_cur_idx;
        end_port = asr_hw->tx_data_cur_idx;
        *io_addr |= (1 << port_idx++);
        *port_num = 1;
        ava_pkt_num--;
        *bitmap_record |= (1 << asr_hw->tx_data_cur_idx);

        if ((++asr_hw->tx_data_cur_idx) == 16) {
            asr_hw->tx_data_cur_idx = 1;
            if (ava_pkt_num) {
                //ava_pkt_num--;
                *io_addr &= ~(1 << port_idx++);
            }
        }
    } else {
        return 0;
    }

    while (ava_pkt_num--) {
        if (bitmap & (1 << asr_hw->tx_data_cur_idx)) {
            *bitmap_record |= (1 << asr_hw->tx_data_cur_idx);
            end_port = asr_hw->tx_data_cur_idx;
            (*port_num)++;
            *io_addr |= (1 << port_idx++);
            if ((++asr_hw->tx_data_cur_idx) == 16) {
                asr_hw->tx_data_cur_idx = 1;
                if (ava_pkt_num) {
                    //ava_pkt_num--;
                    *io_addr &= ~(1 << port_idx++);
                }
            }
        } else
            break;
        if (port_idx >= tx_aggr)
            break;
    }
    if (*port_num == 1)
        *io_addr = asr_hw->ioport | start_port;
    else
        *io_addr = asr_hw->ioport | 0x1000 | start_port | ((*io_addr) << 4);

    return *port_num;
#else
    bitmap = asr_hw->tx_use_bitmap;

    if (bitmap & (1 << asr_hw->tx_data_cur_idx)) {
        *start_port = asr_hw->tx_data_cur_idx;
        *end_port = asr_hw->tx_data_cur_idx;

        if (++asr_hw->tx_data_cur_idx == 16)
            asr_hw->tx_data_cur_idx = 1;
        if (ava_pkt_num > tx_aggr)
            *port_num = tx_aggr;
        else
            *port_num = ava_pkt_num;
    } else {
        *port_num = 0;
    }
    return *port_num;
#endif
}

bool asr_sdio_tx_msg_port_available(struct asr_hw * asr_hw)
{
    bool ret;
    asr_rtos_lock_mutex(&asr_hw->tx_msg_lock);
    ret = (asr_hw->tx_use_bitmap & (1 << SDIO_PORT_IDX(0))) ? true : false;
    asr_hw->tx_use_bitmap &= ~(1 << SDIO_PORT_IDX(0));
    asr_rtos_unlock_mutex(&asr_hw->tx_msg_lock);
    return ret;
}

int asr_sdio_send_data(struct asr_hw *asr_hw,uint8_t type,uint8_t* src,uint16_t len,uint32_t io_addr,uint16_t bitmap_record)
{
    int ret = 0;
    static int times = 0;

    if(type == HIF_TX_MSG)//msg
    {
        int count = 20;

        if(times==0)//first time,poll state
        {
            times++;
            ret = poll_card_status(C2H_DNLD_CARD_RDY | C2H_IO_RDY);
            if (ret) {
                dbg(D_ERR, D_UWIFI_CTRL, "%s card is not in rdy\n", __func__);
                return -EBUSY;
            }
        }
        else
        {
            while ((!(asr_sdio_tx_msg_port_available(asr_hw))) && (--count))    //no available msg port 0
            {
                asr_rtos_delay_milliseconds(1);
            }

            if (count == 0) {
                dbg(D_ERR, D_UWIFI_CTRL, "ERROR: msg port 0 busy! wbm=0x%x\n",asr_hw->tx_use_bitmap);
                return -EBUSY;
            }
         }

        //adjust len, must be block size blocks
        len = len + 4; //  4 is for end token

        len = ASR_ALIGN_BLKSZ_HI(len);

        #if 0
        {
            uint32_t *temp = (uint32_t*)src;

            if (src)
                dbg(D_ERR, D_UWIFI_CTRL, "tx msg %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",temp[0],temp[1],temp[2],temp[3],temp[4],temp[5],temp[6],temp[7],temp[8],temp[9],temp[10],temp[11],temp[12],temp[13],temp[14],temp[15]);
            else
                dbg(D_ERR, D_UWIFI_CTRL, "src is null\n");
        }
        #endif
        ret = asr_sdio_tx_common_port_dispatch(asr_hw, src, len, io_addr, 0x1);
    }
    else//data
    {
        if (len % SDIO_BLOCK_SIZE)
            dbg(D_ERR, D_UWIFI_CTRL, "data size:%d is not %d aligned\r\n", len, SDIO_BLOCK_SIZE);

        //dbg(D_ERR, D_UWIFI_CTRL, "[%s] transfer start:%d end:%d aggr_num:%d src:0x%x len:%d", __func__, start_port, end_port, (start_port>end_port) ?(start_port - end_port + 1):(end_port-start_port +1),src, len);
        ret = asr_sdio_tx_common_port_dispatch(asr_hw, src, len, io_addr, bitmap_record);
    }

    return ret;
}


/*
    download_firmware: download firmware into card
    total data format will be: | header data | fw_data | padding | CRC |(CRC check fw_data + padding)
    header data will be: | fw_len | transfer unit | transfer times |
*/
extern unsigned int sdio_get_block_size(void);
#if 0
struct asr_hw* uwifi_get_asr_hw(void)
{
    return sp_asr_hw;
}
#endif
#ifdef SDIO_CLOSE_PS
static int _first_download = 0;
#endif

#if defined(CONFIG_ASR5505) || defined(CONFIG_ASR595X)

int asr_sdio_download_firmware(struct asr_hw *asr_hw, uint8_t *fw_img_data, uint32_t fw_img_size)
{
    #ifdef SDIO_CLOSE_PS
    if(_first_download ==0)
    {
    #endif
    int ret;
    uint32_t header_data[3];
    uint32_t total_len, pad_len;
    //int sdio2host_gpio_no;
    uint8_t *fw_buf, *temp_fw_buf;
    uint8_t padding_data[4] = {0};
    uint8_t *tempbuf = NULL;
    uint8_t crc_len;
    uint32_t fw_crc = 0;
    int retry_times = 3;
    int blk_size;

    blk_size = sdio_get_block_size() * 2;// /2 to change to byte mode; *2 change to block mode

    // unused:
    //sdio2host_gpio_no = get_sdio2host_gpio_no();
    /*step 0 prepare header data and other initializations*/
    pad_len = (fw_img_size % 4) ? (4 - (fw_img_size % 4)) : 0;
    crc_len = 4;
    total_len = fw_img_size + pad_len + crc_len;
    //total_len = ASR_ALIGN_DLDBLKSZ_HI(total_len);
    total_len = ((total_len + blk_size - 1) & (~(blk_size - 1)));
    //blk_size = total_len;
    header_data[0] = fw_img_size + pad_len;
    header_data[1] = blk_size;
    header_data[2] = (total_len)/blk_size;

    //buffer alloc and not align_addr
    temp_fw_buf = fw_buf = (uint8_t *)asr_rtos_malloc(blk_size + sdio_get_block_size());
    if (fw_buf == NULL) {
        dbg(D_ERR, D_UWIFI_CTRL, "alloc buffer failed\r\n");
        return -1;
    }
    fw_buf = (uint8_t *)(ALIGN_ADDR(fw_buf, DMA_ALIGNMENT));
retry:
    fw_download_done = 0;
    if (!retry_times)
        goto exit;
    dbg(D_DBG, D_UWIFI_CTRL, "%s retry %d times", __func__, 3 - retry_times);
    retry_times--;
    {
        memcpy(fw_buf, header_data, sizeof(header_data));
        //memcpy(fw_img_data + fw_img_size, padding_data, pad_len);
        /*calculate CRC, data parts include the padding data*/
        fw_crc = 0;//crc32(fw_img_data, fw_img_size + pad_len);
        //memcpy(fw_img_data + fw_img_size + pad_len, &fw_crc, crc_len);
        dbg(D_ERR, D_UWIFI_CTRL, "fw_len:%d pad_len:%d crc_len:%d total_len:%d headers: 0x%x-0x%x-0x%x crc:0x%x blk_size:%d\r\n",
            fw_img_size, pad_len, crc_len, total_len, header_data[0], header_data[1], header_data[2], fw_crc, blk_size);
    }
    /*step 1 polling if card is in rdy*/
    ret = poll_card_status(C2H_DNLD_CARD_RDY | C2H_IO_RDY);
    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s card is not in rdy\n", __func__);
        goto exit;
    }

    /*step 2 send out header data*/
    ret = asr_sdio_tx_common_port_dispatch(asr_hw, fw_buf, sizeof(header_data), (asr_hw->ioport | 0x0), 0x1);
    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s write header data failed %d\n", __func__, ret);
        goto exit;
    }

    /*step 3 send out fw data*/
    //memcpy(fw_buf, fw_img_data, total_len);
    int i = 0;

    do {
#ifdef STM32H7_SDK
        extern void asr_usleep(uint32_t time_us);
        asr_usleep(10);
#endif

        //use msg port 0, still polling
        ret = poll_card_status(C2H_DNLD_CARD_RDY | C2H_IO_RDY);
        if (ret) {
            dbg(D_ERR, D_UWIFI_CTRL, "%s card is not in rdy\n", __func__);
            goto exit;
        }
        memcpy(fw_buf, fw_img_data+i*blk_size, blk_size);
        fw_crc = crc32_recursive(fw_buf, blk_size, fw_crc);
        ret = asr_sdio_tx_common_port_dispatch(asr_hw, fw_buf, blk_size, (asr_hw->ioport | 0x0), 0x1);
        if (ret) {
            dbg(D_ERR, D_UWIFI_CTRL, "%s can't write %d data into card %d\n", __func__, total_len, ret);
            goto exit;
        }

        /*
        if (blk_size >= 2 * sdio_get_block_size())
            dbg(D_ERR, D_UWIFI_CTRL, "send %d bytes in block mode [%d]\r\n", blk_size, i);
        else
            dbg(D_ERR, D_UWIFI_CTRL, "send %d bytes in byte mode [%d]\r\n", blk_size, i);
        */
        i++;
    } while (i < ((total_len / blk_size) - 1));
    //use msg port 0, still polling
    ret = poll_card_status(C2H_DNLD_CARD_RDY | C2H_IO_RDY);
    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s card is not in rdy\n", __func__);
        goto exit;
    }
    memcpy(fw_buf, fw_img_data+i*blk_size, fw_img_size - i*blk_size);
    fw_crc = crc32_recursive(fw_buf, fw_img_size - i*blk_size, fw_crc);
    memcpy(fw_buf + fw_img_size - i*blk_size, padding_data, pad_len);
    dbg(D_DBG, D_UWIFI_CTRL, "crc:0x%x", fw_crc);
    memcpy(fw_buf + fw_img_size - i*blk_size + pad_len, &fw_crc, crc_len);
    ret = asr_sdio_tx_common_port_dispatch(asr_hw, fw_buf, blk_size, (asr_hw->ioport | 0x0), 0x1);
    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s can't write %d data into card %d\n", __func__, total_len, ret);
        goto exit;
    }

    #ifdef SDIO_CLOSE_PS
    }
    #endif
    /*
        sdio card side will reset sdio for some next transfer, otherwise sdio fw boot up can't receive message
        if sdio card do reset operation, it need re-do initialization after transfer image done for cmd53
    */
    #if 1

    fw_download_done = 1;

    #if 0   // no need to reinit sdio again.
    sdio_host_enable_isr(1);
    asr_sdio_set_block_size(SDIO_BLOCK_SIZE_DLD);//is a question, it should set the hardware of the block size
    sdio_enable_func(1);
    asr_sdio_init_config(asr_hw);
    #endif

    #ifdef SDIO_CLOSE_PS
    if(_first_download ==0)
    {
    _first_download = 1;
    #endif

    /*step 4 check fw_crc status*/
    if (check_scratch_status(CRC_SUCCESS) < 0)
        goto retry;
    /*step 5 check fw runing status*/
    if (check_scratch_status(BOOT_SUCCESS) < 0)
        goto exit;

    dbg(D_ERR, D_UWIFI_CTRL, "BOOT_SUCCESS:0x%x\n", BOOT_SUCCESS);

    #ifndef SDIO_CLOSE_PS
    asr_rtos_free(temp_fw_buf);
    #endif

    #ifdef SDIO_CLOSE_PS
    }
    #endif

    return 0;
    #endif

exit:
    return -1;
}

#elif defined(CONFIG_ASR5825)

#define ASR_SND_BOOT_LOADER_SIZE   1024
/*
    Download fw README
    1, lega/duet chip integrate romcode v1.0, which support download directly into one continuous "ram" region
    2, canon chip intergrate romcode v2.0, which support download different sections of firmware into non-continuous "ram" region

    Now duet with wifi + ble function's firmware have different sections, and its "ram" region is non-continuous,
    but it integrated the romcode v1.0. so for duet support non-continuous "ram" region,
    it needs using 1st_firmware download the snd_bootloader, then using 2nd_firmware download work with snd_bootloader
    to download the total firmware with different section into different "ram" region.

*/
/*
    download_firmware: download firmware into card
    total data format will be: | header data | fw_data | padding | CRC |(CRC check fw_data + padding)
    header data will be: | fw_len | transfer unit | transfer times |
*/

static int asr_sdio_send_fw(struct asr_hw *asr_hw, uint8_t *fw_img_data, uint32_t fw_img_size,
    u8 *fw_buf, int blk_size, u32 total_len, u32 pad_len, bool cal_crc)
{
    int ret = 0;
    int i = 0;
    u32 fw_crc = 0;
    u8 padding_data[4] = { 0 };


    while (i < total_len / blk_size ){
#ifdef STM32H7_SDK
        extern void asr_usleep(uint32_t time_us);
        asr_usleep(5);
#endif
        //use msg port 0, still polling
        ret = poll_card_status(C2H_DNLD_CARD_RDY | C2H_IO_RDY);
        if (ret) {
            dbg(D_ERR, D_UWIFI_CTRL, "%s card is not in rdy\n", __func__);
            return ret;
        }

        memcpy(fw_buf, fw_img_data+i*blk_size, blk_size);
        if (cal_crc) {
            fw_crc = crc32_recursive(fw_buf, blk_size, fw_crc);
        }
        ret = asr_sdio_tx_common_port_dispatch(asr_hw, fw_buf, blk_size, (asr_hw->ioport | 0x0), 0x1);
        if (ret) {
            dbg(D_ERR, D_UWIFI_CTRL, "%s can't write %d data into card %d\n", __func__, total_len, ret);
            return ret;
        }

        #if 0
        if (blk_size >= 2 * sdio_get_block_size())
            dbg(D_ERR, D_UWIFI_CTRL, "send %d bytes in block mode [%d]", blk_size, i);
        else
            dbg(D_ERR, D_UWIFI_CTRL, "send %d bytes in byte mode [%d]", blk_size, i);
        #endif
        i++;
    }

    if (!(total_len - i*blk_size || pad_len || cal_crc)) {
        return ret;
    }

    memset(fw_buf, 0, blk_size);

    dbg(D_ERR, D_UWIFI_CTRL, "%s:fw_img_size=%d,len=%d,fw_crc=0x%X\n",
        __func__, fw_img_size, total_len - i*blk_size, fw_crc);

    //use msg port 0, still polling
    ret = poll_card_status(C2H_DNLD_CARD_RDY | C2H_IO_RDY);
    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s card is not in rdy\n", __func__);
        return ret;
    }

    if (fw_img_size - i*blk_size != 0) {
        memcpy(fw_buf, fw_img_data+i*blk_size, fw_img_size - i*blk_size);
    }

    if (pad_len) {
        memcpy(fw_buf + fw_img_size - i*blk_size, padding_data, pad_len);
    }

    if (cal_crc) {
        fw_crc = crc32_recursive(fw_buf, fw_img_size - i*blk_size, fw_crc);
        dbg(D_DBG, D_UWIFI_CTRL, "CRC:0x%x\n", fw_crc);
        memcpy(fw_buf + fw_img_size - i*blk_size + pad_len, &fw_crc, sizeof(fw_crc));
    }

    ret = asr_sdio_tx_common_port_dispatch(asr_hw, fw_buf, blk_size, (asr_hw->ioport | 0x0), 0x1);
    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s can't write %d data into card %d\n", __func__, total_len, ret);
        return ret;
    }

    return ret;
}

static int asr_sdio_download_1st_firmware(struct asr_hw *asr_hw, uint8_t *fw_img_data, uint32_t fw_img_size)
{
    int ret = 0;
    u32 header_data[3];
    u32 fw_len, total_len, pad_len = 0, crc_len = 4;
    int blk_size;
    u8 *temp_fw_buf = NULL;
    u8 *fw_buf = NULL;

    blk_size = sdio_get_block_size() * 2;// /2 to change to byte mode; *2 change to block mode

    /*step 0 prepare header data and other initializations */
    fw_len = ASR_SND_BOOT_LOADER_SIZE;
    pad_len = (fw_len % 4) ? (4 - (fw_len % 4)) : 0;
    total_len = fw_len + pad_len + crc_len;
    total_len = ASR_ALIGN_DLDBLKSZ_HI(total_len);
    header_data[0] = ASR_SND_BOOT_LOADER_SIZE + pad_len;
    header_data[1] = blk_size;
    header_data[2] = total_len / blk_size;
    if (total_len % blk_size) {
       header_data[2]++;
    }

    //buffer alloc and not align_addr
    temp_fw_buf = fw_buf = (uint8_t *)asr_rtos_malloc(blk_size + sdio_get_block_size());
    if (fw_buf == NULL) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:alloc buffer failed\r\n", __func__);
        return -ENOMEM;
    }
    fw_buf = (uint8_t *)(ALIGN_ADDR(fw_buf, DMA_ALIGNMENT));


    memcpy(fw_buf, (u8 *) header_data, sizeof(header_data));

    dbg(D_INF, D_UWIFI_CTRL,
        "%s fw_len:%d total_len:%d blk_size:%d headers: 0x%x-0x%x-0x%x\n", __func__,
        fw_len, total_len, blk_size, header_data[0], header_data[1], header_data[2]);

    /*step 1 polling if card is in rdy */
    ret = poll_card_status(C2H_DNLD_CARD_RDY | C2H_IO_RDY);
    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s card is not in rdy\n", __func__);
        goto exit_1st_fw;
    }

    /*step 2 send out header data */
    ret = asr_sdio_tx_common_port_dispatch(asr_hw, fw_buf, sizeof(header_data), (asr_hw->ioport | 0x0), 0x1);
    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s write header data failed %d\n", __func__, ret);
        goto exit_1st_fw;
    }

    /*step 3 send out fw data */
    ret = asr_sdio_send_fw(asr_hw, fw_img_data, fw_len, fw_buf, blk_size, total_len, pad_len, true);
    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s can't write %d data into card %d\n", __func__, total_len, ret);
        goto exit_1st_fw;
    }

    /*step 4 check fw_crc status */
    //if (check_scratch_status(func, CRC_SUCCESS) < 0)
    //    ret = -1;
    //    goto exit_1st_fw;
    //}
    //dev_info(g_asr_para.dev, "%s CRC_SUCCESS:0x%x\n", __func__, CRC_SUCCESS);

    /*step 5 check fw runing status */
    if (check_scratch_status(BOOT_SUCCESS) < 0) {
        ret = -1;
        goto exit_1st_fw;
    }
    dbg(D_INF, D_UWIFI_CTRL, "%s BOOT_SUCCESS:0x%x\n", __func__, BOOT_SUCCESS);

    ret = 0;

exit_1st_fw:
    if (temp_fw_buf) {
        asr_rtos_free(temp_fw_buf);
        temp_fw_buf = NULL;
    }

    return ret;
}

static int asr_sdio_clear_status(uint16_t status)
{
    int ret = 0;
    uint8_t scratch_val;

    asr_sdio_claim_host();
    ret = sdio_readb_cmd52(SCRATCH_1, &scratch_val);
    asr_sdio_release_host();
    if (!ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "can't read 0x%x out\n",  SCRATCH_1);
        return -1;
    }

    asr_sdio_claim_host();
    ret = sdio_writeb_cmd52(SCRATCH_1, scratch_val | ~(status >> 8));
    asr_sdio_release_host();
    if (!ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s write fw_sec_num  fail!!! (%d)\n", __func__, ret);
        return -1;
    }
    dbg(D_INF, D_UWIFI_CTRL, "%s clear 0x%X\n", __func__, status);

    return 0;
}

static int asr_sdio_send_section_firmware(struct asr_hw *asr_hw, struct sdio_host_sec_fw_header
                   *p_sec_fw_hdr, u8 * fw_buf, u8 *fw_img_data, int blk_size)
{
    int ret = 0;

    /*step 1 polling if card is in rdy */
    ret = poll_card_status(C2H_DNLD_CARD_RDY | C2H_IO_RDY);
    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s card is not in rdy\n", __func__);
        return ret;
    }

    ret = asr_sdio_clear_status(CRC_SUCCESS);
    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s clear crc fail\n", __func__);
        return ret;
    }

    /*step 2 send out header data */
    memmove(fw_buf, p_sec_fw_hdr, sizeof(struct sdio_host_sec_fw_header));
    ret = asr_sdio_tx_common_port_dispatch(asr_hw, fw_buf, sizeof(struct sdio_host_sec_fw_header), (asr_hw->ioport | 0x0), 0x1);
    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s write header data failed %d\n", __func__, ret);
        return ret;
    }

    /*step 3 send out fw data */
    ret = asr_sdio_send_fw(asr_hw, fw_img_data, p_sec_fw_hdr->sec_fw_len, fw_buf, blk_size,
        p_sec_fw_hdr->sec_fw_len, 0, false);
    if (ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s can't write %d data into card %d\n", __func__, p_sec_fw_hdr->sec_fw_len, ret);
        return ret;
    }

    /*step 4 check fw_crc status */
    if (check_scratch_status(CRC_SUCCESS) == 0) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s CRC_SUCCESS:0x%x\n", __func__, CRC_SUCCESS);
    } else {
        return -1;
    }

    return ret;
}

#define HYBRID_BIN_TAG 0xAABBCCDD
static int asr_sdio_download_2nd_firmware(struct asr_hw *asr_hw, uint8_t *fw_img_data, uint32_t fw_img_size)
{
    u8 *temp_fw_buf = NULL;
    u8 *fw_buf = NULL;
    u8 *fw_data;
    size_t fw_size;
    int ret = 0;
    struct sdio_host_sec_fw_header sdio_host_sec_fw_header = { 0 };
    u32 total_sec_num, fw_tag, sec_idx, fw_hybrid_header_len, fw_sec_total_len, fw_dled_len;
    u8 fw_sec_num;
    int blk_size;

    blk_size = sdio_get_block_size() * 2;// /2 to change to byte mode; *2 change to block mode

    /*parse fw header and get sec num */
    fw_data = fw_img_data + ASR_SND_BOOT_LOADER_SIZE;
    fw_size = fw_img_size - ASR_SND_BOOT_LOADER_SIZE;
    memcpy(&fw_tag, fw_data, sizeof(u32));

    if (HYBRID_BIN_TAG != fw_tag) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s fw tag mismatch(0x%x 0x%x)\n", __func__, HYBRID_BIN_TAG, fw_tag);
        ret = -EINVAL;
        goto exit_2nd_fw;
    }
    memcpy(&total_sec_num, fw_data + 4, sizeof(u32));
    fw_sec_num = (u8) total_sec_num;
    // write sec num to scra register for fw read.
    asr_sdio_claim_host();
    ret = sdio_writeb_cmd52(SCRATCH0_3, fw_sec_num);
    ret = sdio_writeb_cmd52(SCRATCH1_0, asr_hw->mac_addr[0]);
    ret = sdio_writeb_cmd52(SCRATCH1_1, asr_hw->mac_addr[1]);
    ret = sdio_writeb_cmd52(SCRATCH1_2, asr_hw->mac_addr[2]);
    ret = sdio_writeb_cmd52(SCRATCH1_3, asr_hw->mac_addr[5]);
    asr_sdio_release_host();
    if (!ret) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s write fw_sec_num  fail!!! (%d)\n", __func__, ret);
        goto exit_2nd_fw;
    }
    // tag + sec num + [len/crc/addr]...+
    fw_hybrid_header_len = 8 + total_sec_num * sizeof(struct sdio_fw_sec_fw_header);
    fw_sec_total_len = fw_size - fw_hybrid_header_len;
    if (fw_sec_total_len % 4) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s error: fw bin length not 4byte allignment\n", __func__);
        ret = -EINVAL;
        goto exit_2nd_fw;
    }
    dbg(D_INF, D_UWIFI_CTRL, "%s hybrid hdr (0x%x %d %d %d)!!\n", __func__, fw_tag,
        total_sec_num, fw_hybrid_header_len, fw_sec_total_len);

    //buffer alloc and not align_addr
    temp_fw_buf = fw_buf = (uint8_t *)asr_rtos_malloc(blk_size + sdio_get_block_size());
    if (fw_buf == NULL) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s:alloc buffer failed\r\n", __func__);
        ret = -ENOMEM;
        goto exit_2nd_fw;
    }
    fw_buf = (uint8_t *)(ALIGN_ADDR(fw_buf, DMA_ALIGNMENT));

    sec_idx = 0;
    fw_dled_len = 0;
    do {
        // prepare sdio_host_sec_fw_header
        memcpy(&sdio_host_sec_fw_header, fw_data + 8 +
               sec_idx * sizeof(struct sdio_fw_sec_fw_header), sizeof(struct sdio_fw_sec_fw_header));

        sdio_host_sec_fw_header.transfer_unit = blk_size;
        sdio_host_sec_fw_header.transfer_times = sdio_host_sec_fw_header.sec_fw_len / blk_size;

        if (sdio_host_sec_fw_header.sec_fw_len % blk_size) {
            sdio_host_sec_fw_header.transfer_times++;
        }
        dbg(D_INF, D_UWIFI_CTRL, "%s idx(%d),dled_len=%d,sec headers: 0x%x-0x%x-0x%x-0x%x-0x%x\n",
             __func__, sec_idx, fw_dled_len,
             sdio_host_sec_fw_header.sec_fw_len,
             sdio_host_sec_fw_header.sec_crc,
             sdio_host_sec_fw_header.chip_ram_addr,
             sdio_host_sec_fw_header.transfer_unit, sdio_host_sec_fw_header.transfer_times);


        if (asr_sdio_send_section_firmware(asr_hw, &sdio_host_sec_fw_header, fw_buf,
            fw_data + fw_hybrid_header_len + fw_dled_len, blk_size)) {
            // any section fail,exit
            dbg(D_ERR, D_UWIFI_CTRL, "%s sec(%d) download fail!\n", __func__, sec_idx);
            ret = -1;
            goto exit_2nd_fw;
        }

        // caculate downloaded bin length.
        fw_dled_len += sdio_host_sec_fw_header.sec_fw_len;
        sec_idx++;
    } while (sec_idx < total_sec_num);

    if (fw_dled_len != fw_sec_total_len) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s fw len mismatch(%d %d)\n", __func__, fw_dled_len, fw_sec_total_len);
        ret = -1;
        goto exit_2nd_fw;
    }
    /*step 5 check fw runing status */
    if (check_scratch_status(BOOT_SUCCESS) < 0) {
        ret = -1;
        goto exit_2nd_fw;
    }

    asr_msleep(20);
    dbg(D_INF, D_UWIFI_CTRL, "%s BOOT_SUCCESS:0x%x\n", __func__, BOOT_SUCCESS);

    if (poll_card_status(C2H_IO_RDY)) {
        dbg(D_ERR, D_UWIFI_CTRL, "%s card is not in rdy\n", __func__);
        ret = -1;
        goto exit_2nd_fw;
    }

    ret = 0;

exit_2nd_fw:
    if (temp_fw_buf) {
        asr_rtos_free(temp_fw_buf);
        temp_fw_buf = NULL;
    }

    return ret;
}

int asr_sdio_download_firmware(struct asr_hw *asr_hw, uint8_t *fw_img_data, uint32_t fw_img_size)
{
    int ret = 0;

    //download 2nd bootloader work with romcode v1.0
    ret = asr_sdio_download_1st_firmware(asr_hw, fw_img_data, fw_img_size);
    if (ret != 0) {
        return ret;
    }

    //download the hybrid firmware work with the 2nd bootloader
    ret = asr_sdio_download_2nd_firmware(asr_hw, fw_img_data, fw_img_size);

    return ret;
}

#endif

