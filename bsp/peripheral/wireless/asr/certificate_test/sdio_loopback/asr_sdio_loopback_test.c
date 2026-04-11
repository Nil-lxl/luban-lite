/*
 * ASR SDIO driver
 *
 * Copyright (C) 2017, ASRmicro Technology.
 *
 */
#ifdef SDIO_LOOPBACK_TEST
#include <stdint.h>
#include <stdio.h>
#include "uwifi_sdio.h"
#include "asr_rtos_api.h"
#include "asr_types.h"
#include "asr_sdio.h"


 /* SDK private APIs */

extern uint8_t sdio_read_cmd53(uint32_t dataport, uint8_t *data, int size);
extern uint8_t sdio_write_cmd53(uint32_t dataport, uint8_t *data, int size);
extern void asr_sdio_claim_host(void);
extern void asr_sdio_release_host(void);
extern int poll_card_status(uint8_t mask);

/* SDK private APIs END*/

int g_use_irq_flag = 1;

static int start_flag = 0;
static char *fw_name = NULL;
static int loopback_times = 1;
static int loopback_test_mode = 0x7;
static int direct_block_size = 0;
static bool is_loopback_test = 0;
static int func_number = 0;
static bool is_loopback_display = 0;
static bool direct_pass = 0;
static int crc_value = 0;
static int is_host_int = 0;
static int test_irq_count = 100;
int card_indication_gpio;
int test_gpio;
int card_irq;
static int gpio_irq_times;
asr_semaphore_t dnld_completion;
asr_semaphore_t upld_completion;
asr_mutex_t dnld_mutex;
asr_mutex_t upld_mutex;

typedef struct port_buf{
	void *buf;
	int len;
} port_buf_t;
port_buf_t *port_buf[16];
/*every time set_block_size need update this variable*/
static uint32_t g_block_size = 0;

asr_thread_hand_t Irq_status_Task_Handler = {0};
asr_mutex_t sdio_irq_mutex;
asr_event_t   sdio_loopback_event;


#define SDIO_LOOPBACK_EVENT_INT     (0x1 << 0)


static uint32_t asr_crc32(const uint8_t *buf, uint32_t len)
{
	uint32_t i;
	uint32_t crc = 0xffffffff;

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

int ffs1(u16 i)
{
    int index;

    for(index = 0; index < 16; index++)
    {
        if (i & (1 << index))
        {
            return index + 1;
        }
    }

    return 0;
}

int fls1(u16 i)
{
    int index;

    for(index = 15; index >= 0; index--)
    {
        if (i & (1 << index))
        {
            return index + 1;
        }
    }

    return 0;
}

#define SDIO_LOOPBACK_DEBUG 1
#ifdef SDIO_LOOPBACK_DEBUG

#define loopback_log(fmt, arg...) \
	printf("%s[%d]: "fmt"\r\n", __FUNCTION__, __LINE__, ##arg)
#else
#define loopback_log(fmt, arg...) \
	do {	\
	} while (0)
#endif

void *single_buf = NULL;
u16 single_buf_size = 0;

unsigned char *multi_port_buffer[16];
u16 multi_port_size[16] = {0};

void *aggr_rd_buf = NULL;
void *aggr_wr_buf = NULL;
int aggr_rd_size = 0;
int aggr_wr_size = 0;
u16 aggr_port_size[16] = {0};
u16 aggr_port_size_print[16] = {0};


int asr_sdio_readsb(unsigned int addr, void *dst, uint32_t count)
{
    uint32_t size = count;
    uint32_t rx_blocks = 0, blksize = 0;
    int ret = 0;

    asr_sdio_claim_host();
    ret = sdio_read_cmd53(addr, (unsigned char *)dst, size);
    asr_sdio_release_host();
    if (0 != ret)
    {
        loopback_log("read_cmd53 read failed, ret = %d", ret);
    }

    return ret;
}

int asr_sdio_writesb(unsigned int addr, void *src, uint32_t count)
{
    unsigned int size = count;
    uint32_t tx_blocks = 0, blksize = 0;
    int ret = 0;

    asr_sdio_claim_host();
    ret = sdio_write_cmd53(addr, (unsigned char *)src, size);
    asr_sdio_release_host();
    if (0 != ret)
    {
        loopback_log("write_cmd53 write failed, ret = %d", ret);
    }

    return ret;
}

/*
	read_write_single_port_test: single port read or write
	write: 0 means read; 1 means write
*/
static void trigger_slaver_do_rxtest(asr_drv_sdio_t *func)
{
    int ret = 0;
    uint8_t reg_val = 0;

    reg_val = (0x1 << 3);
    sdio_writeb_cmd52(H2C_INTEVENT, reg_val);
    printf("Send H2C_INTEVENT done, ret: %d,%u\n", ret, asr_rtos_get_time());

    return;
}

static int read_write_single_port_test(asr_drv_sdio_t *func, bool write)
{
	int ioport = 0, ret = 0, i;
	uint32_t addr;
    unsigned char sdio_info_arr[64] = {0};
	u16 port_index = 0, bitmap;
	u8 reg_val;

	/*step 0: get ioport*/
	ioport = sdio_get_ioport();
	if (ioport < 0) {
		loopback_log("get ioport failed");
		return -ENODEV;
	}
	//port address occpy [17:14]
	ioport &= (0x1e000);

	/*step 1: wait for card rdy now only using irq way*/
	/* Check card status by polling or interrupt mode */
	if (start_flag) {
    	if (g_use_irq_flag == 1) {
    		if (write) {
                asr_rtos_get_semaphore(&dnld_completion,ASR_WAIT_FOREVER);
            }
    		else {//read
                asr_rtos_get_semaphore(&upld_completion,ASR_WAIT_FOREVER);
            }
    	} else if (g_use_irq_flag == 2) {
    		loopback_log("Do not support irq 2 mode.");
    	} else {
    	    loopback_log("Polling SDIO card status.");
    		ret = poll_card_status((write ? C2H_DNLD_CARD_RDY : C2H_UPLD_CARD_RDY) | C2H_IO_RDY);
    		if (ret) {
    			loopback_log("SDIO device not ready!\n");
    			return -ENODEV;
    		}
    	}
	} else {
	    start_flag = 1;
	    loopback_log("first Polling SDIO card status.");
		ret = poll_card_status((write ? C2H_DNLD_CARD_RDY : C2H_UPLD_CARD_RDY) | C2H_IO_RDY);
		if (ret) {
			loopback_log("SDIO device not ready!\n");
			start_flag = 0;
			return -ENODEV;
		}
    }

    asr_sdio_claim_host();
    ret = asr_sdio_read_cmd53(H2C_INTEVENT, sdio_info_arr, 64);
    asr_sdio_release_host();
    if (ret) {
        loopback_log("read sdio info failed.");
        return -ENODEV;
    }

	if (!write) {
        bitmap = sdio_info_arr[RD_BITMAP_U] << 8 | sdio_info_arr[RD_BITMAP_L];
	} else {
        bitmap = sdio_info_arr[WR_BITMAP_U] << 8 | sdio_info_arr[WR_BITMAP_L];
	}

	port_index = ffs1(bitmap) - 1;
    loopback_log("bitmap:0x%x:%d, port: %d", bitmap, bitmap, port_index);

	if (!write) {
	    single_buf_size = sdio_info_arr[RD_LEN_U(port_index)] << 8 | sdio_info_arr[RD_LEN_L(port_index)];

		if (single_buf_size == 0) {
			loopback_log("single port test card->host return buf size is 0, error!");
			return -ENODEV;
		}
		single_buf = asr_rtos_malloc((unsigned int)single_buf_size);
		if (!single_buf) {
			loopback_log("alloc buffer failed, size: %d", single_buf_size);
			return -ENOMEM;
		}
	} else {
		if (!single_buf || single_buf_size == 0) {
			loopback_log("Write failed, buf is null or buf len is 0");
			return -ENODEV;
		}
	}

	/*step 2: do a read or write operation*/
	addr = ioport | (port_index);
	loopback_log("CMD53 %s, addr:0x%x port:%d (bitmap:0x%x) buf len:%d (from rd len register[%d]:0x%x)\n",
    			write ? "W ->" : "R <-", addr,
    			port_index, bitmap, single_buf_size,
    			port_index, RD_LEN_L(port_index));

    ret = write ? asr_sdio_writesb(addr, (unsigned char *)single_buf, single_buf_size) : asr_sdio_readsb(addr, (unsigned char *)single_buf, single_buf_size);
    if (ret) {
        loopback_log("%s data failed\n", write ? "write" : "read");
        goto err;
    }

	loopback_log("%s Done, last data:0x%x", write?"W":"R", *((unsigned char *)single_buf + (single_buf_size - 1)));

    if (!write)
    {
#if 0
    	print_hex_dump(KERN_DEBUG, "RX: ", DUMP_PREFIX_OFFSET, 16, 1,
    		        single_buf, single_buf_size, true);
        //*((uint8_t*)single_buf) = 1;
#endif
    }

err:
	//free buffer after write cycle
	if (write) {
		asr_rtos_free(single_buf);
		single_buf = NULL;
		single_buf_size = 0;
	}

	return ret;
}

/*
	single_port_tests:
	return 0 as success, return < 0 as fail;
*/
static int single_port_tests(asr_drv_sdio_t *func)
{
    // same in device side
	int block_size_arr[] = {16, 32, 256};
	int loop, test_times, ret;

	for (loop = 0; loop < ARRAY_SIZE(block_size_arr); loop++) {

        g_block_size = block_size_arr[loop];
		ret = asr_sdio_set_block_size(block_size_arr[loop]);
		if (ret) {
			loopback_log("Set block size failed");
			return ret;
		}
		loopback_log("Start single port test...");

		// loop test for each port
		for (test_times = 0; test_times < 16; test_times++) {
		    loopback_log("single port test: block size: %d, cycle: %d\r\n", block_size_arr[loop], test_times);

			ret = read_write_single_port_test(func, 0);
			if (ret) {
				loopback_log("read single port test failed");
				return ret;
			}
			ret = read_write_single_port_test(func, 1);
			if (ret) {
				loopback_log("write single port test failed");
				return ret;
			}
		}
	}

	loopback_log("*******************************************\r\n"
	             "*********** signle port test done! ********\r\n"
	             "*******************************************\r\n");

	return 0;
}

static int read_write_multi_port_test(asr_drv_sdio_t *func, bool write)
{
	int ioport, ret, i, j, port_used;
	uint32_t addr;
	u16 port_size;
	u16 bitmap;
	u8 reg_val;
    unsigned char sdio_info_arr[64] = {0};
	u16 port_index = 0;
    u16 data_buf_index = 0;

	/*step 0: get ioport*/
	ioport = sdio_get_ioport();
	if (ioport < 0) {
		loopback_log("get ioport failed\n");
		return -ENODEV;
	}
	ioport &= (0x1e000);

	/*step 1: wait for card rdy*/
	if (g_use_irq_flag == 1) {
		if (write)
            asr_rtos_get_semaphore(&dnld_completion,ASR_WAIT_FOREVER);
		else//read
            asr_rtos_get_semaphore(&upld_completion,ASR_WAIT_FOREVER);
	} else if (g_use_irq_flag == 2) {
		loopback_log("can't support now");
	} else {

        loopback_log("Polling SDIO card status.");
		ret = poll_card_status((write ? C2H_DNLD_CARD_RDY : C2H_UPLD_CARD_RDY) | C2H_IO_RDY);
		if (ret) {
			loopback_log("card is not rdy!");
			return -ENODEV;
		}
	}

    asr_sdio_claim_host();
    ret = asr_sdio_read_cmd53(H2C_INTEVENT, sdio_info_arr, 64);
    asr_sdio_release_host();
    if (ret) {
        loopback_log("read sdio info failed.");
        return -ENODEV;
    }

	if (!write) {
        bitmap = sdio_info_arr[RD_BITMAP_U] << 8 | sdio_info_arr[RD_BITMAP_L];
	} else {
        bitmap = sdio_info_arr[WR_BITMAP_U] << 8 | sdio_info_arr[WR_BITMAP_L];
	}

	port_index = ffs1(bitmap) - 1;
    loopback_log("bitmap: 0x%x, port: %d", bitmap, port_index);

	port_used = 0;

	//do multi read/write operation
	// need make sure read before write
	for (i = 0; i < 16; i++) {
	    //in test phase rd_bitmap can not match the wr_bitmap, but from right to left to assign the buffer to write

	    // find each free port
		if (bitmap & (1 << i)) {

			if (write) {//write

				addr = ioport | i;
			 	port_size  = multi_port_size[port_used];

                ret = asr_sdio_writesb(addr, (unsigned char *)multi_port_buffer[port_used], port_size);
				if (ret) {
					loopback_log("%s data failed\n", "write");
					return ret;
				}

				//free buffer
				asr_rtos_free(multi_port_buffer[port_used]);
				multi_port_buffer[port_used] = NULL;
				multi_port_size[port_used] = 0;
				port_used++;
			} else {//read
	            port_size = sdio_info_arr[RD_LEN_U(i)] << 8 | sdio_info_arr[RD_LEN_L(i)];

				if (port_size == 0) {
					loopback_log("read_write_multi read port(%d) len is 0, error!", i);
					return -ENODEV;
				}

                data_buf_index = 0;
                for (j = 0; j < port_used; j++) {
                    data_buf_index +=  multi_port_size[port_used];
                }

				//alloc buffer
		        multi_port_buffer[port_used] = asr_rtos_malloc((unsigned int)port_size);
		        if (!multi_port_buffer[port_used]) {
			        loopback_log("multi alloc buffer failed, size: %d", port_size);
			        return -ENOMEM;
		        }


				multi_port_size[port_used] = port_size;
				if (!multi_port_buffer[port_used]) {
					for (j = 0; j < port_used; j++) {
						if (multi_port_buffer[port_used])
							asr_rtos_free(multi_port_buffer[port_used]);
					}
					loopback_log("alloc memory failed during multi-port test!, cycle: %d", i);
					return -ENOMEM;
				}

				addr = ioport | i;

                ret = asr_sdio_readsb(addr, (unsigned char *)multi_port_buffer[port_used], port_size);
				if (ret) {
					loopback_log("Read data failed, cycle: %d", i);
					return ret;
				}

				port_used++;
			}

            loopback_log(" %s port:%d(from bitmap:0x%x) addr:%d buf_len:%d (from rd len [%d]:0x%x) port_used:%d",
                    write?"W-> ":"R<- ", i, bitmap, addr, port_size, i, RD_LEN_L(i), port_used);
		}
	}

	return 0;
}


static int multi_port_tests(asr_drv_sdio_t *func)
{
	int loop, test_times;
	int block_size_arr[] = {16, 32, 256};
	int ret;

	for (loop = 0; loop < 16; loop++)
		multi_port_buffer[loop] = NULL;

	// loop 3 different block size with each port
	for (loop = 0; loop < ARRAY_SIZE(block_size_arr); loop++) {
        g_block_size = block_size_arr[loop];
		ret = asr_sdio_set_block_size(block_size_arr[loop]);
		if (ret) {
			loopback_log("%s set block size failed\n", __func__);
			return ret;
		}
		loopback_log("set block_size:%d", block_size_arr[loop]);

		for (test_times = 0; test_times < 16; test_times++) {
			ret = read_write_multi_port_test(func, 0);
			if (ret) {
				loopback_log("%s read port test failed\n", __func__);
				return ret;
			}
			ret = read_write_multi_port_test(func, 1);
			if (ret) {
				loopback_log("%s write port test failed\n", __func__);
				return ret;
			}
		}
	}

	loopback_log("*******************************************\r\n"
	             "*********** multi port test done! ********\r\n"
	             "*******************************************\r\n");

	return 0;
}

static int read_write_agg_port_test(asr_drv_sdio_t *func, int block_size, bool write)
{
	int ioport, ret;
	uint32_t addr;
	int agg_bitmap, i, agg_num;
	int start, end, port_used;
	u16 port_size, bitmap;
	u16 start_port;
	u8 reg_val;
    unsigned char sdio_info_arr[64] = {0};

	/*step 0: get ioport*/
	ioport = sdio_get_ioport();
	if (ioport < 0) {
		loopback_log("%s get ioport failed\n", __func__);
		return -EINVAL;
	}
	ioport &= (0x1e000);

	/*step 1: wait for card rdy*/
	if (g_use_irq_flag == 1) {
		if (write) {
			loopback_log("write_completion start\n");
            asr_rtos_get_semaphore(&dnld_completion,ASR_WAIT_FOREVER);
			loopback_log("write_completion end\n");
		} else {//read
			loopback_log("read_completion start\n");
            asr_rtos_get_semaphore(&upld_completion,ASR_WAIT_FOREVER);
			loopback_log("read_completiion end\n");
		}
	} else if (g_use_irq_flag == 2) {
		loopback_log("can't support now");
	} else {
		ret = poll_card_status((write ? C2H_DNLD_CARD_RDY : C2H_UPLD_CARD_RDY) | C2H_IO_RDY);
		if (ret) {
			loopback_log("card is not rdy!");
			return -ENODEV;
		}
	}

	asr_sdio_claim_host();
    ret = asr_sdio_read_cmd53(H2C_INTEVENT, sdio_info_arr, 64);
    asr_sdio_release_host();
    if (ret) {
        loopback_log("read sdio info failed.");
        return -ENODEV;
    }

	if (!write) {
        bitmap = sdio_info_arr[RD_BITMAP_U] << 8 | sdio_info_arr[RD_BITMAP_L];
	} else {
        bitmap = sdio_info_arr[WR_BITMAP_U] << 8 | sdio_info_arr[WR_BITMAP_L];
	}

	if (!write) {
		aggr_rd_size = 0;
		port_used = 0;
		for (i = 0; i < 16; i++) {
			aggr_port_size[i] = 0;
			aggr_port_size_print[i] = 0;
		}
		loopback_log("aggr_rd_size = 0");

		for (i = 0; i < 16; i++) {
			if (bitmap & (1 << i)) {
	            port_size = sdio_info_arr[RD_LEN_U(i)] << 8 | sdio_info_arr[RD_LEN_L(i)];

				if (port_size == 0) {
					loopback_log("read_write_agg read port(%d) len is 0, error!", i);
					return -ENODEV;
				}

				loopback_log(" + %d(from [%d]:0x%x 0x%x)", port_size, i, RD_LEN_L(i), RD_LEN_U(i));

                // record each port size
				aggr_port_size[port_used] = port_size;
				aggr_port_size_print[i] = port_size;
				port_used++;
				aggr_rd_size += port_size;
			}
		}

		//add 4 fake size to store mark char
		aggr_rd_size += 4;
		loopback_log(";\n");
		aggr_rd_buf = asr_rtos_malloc((unsigned int)aggr_rd_size);
		if (!aggr_rd_buf) {
			loopback_log(" can't allock buffer for aggregation test, length: %d", aggr_rd_size);
			return -ENOMEM;
		}
	} else {//write
		//[2byte length][data][padding] should in block size
		aggr_wr_size = 0;
		port_used = 0;
		loopback_log("aggr_wr_size = 0");
		for (i = 0; i < 16; i++) {
			if (bitmap & (1 << i)) {
				aggr_wr_size += aggr_port_size[port_used];//data size
				loopback_log(" + %d", aggr_port_size[port_used]);
				// aligned with block size
				if ((aggr_port_size[port_used]) % block_size) {
					aggr_wr_size += (block_size - ((aggr_port_size[port_used]) % block_size));
					loopback_log(" + %d", (block_size - ((aggr_port_size[port_used]) % block_size)));
				}
				port_used++;
			}
		}
		loopback_log(";\n");

	}

	//found the bitmap area of aggr mode
	start = ffs1(bitmap);
	end = fls1(bitmap);
	//assume that there is full '1' between ffs and fls, take no care other situations now
	/*
		1st situation

				 >fls - 1<   >ffs - 1<
					  |           |
		--------------------------------
		|             |///////////|    |
		|             |///////////|    |
		|             |///////////|    |
		--------------------------------
		              ^           ^
					  |           |
					 end		 start
		2nd situation

   >fls - 1<					  >ffs - 1<
	    |                              |
		--------------------------------
		|/////|                    |///|
		|/////|       			   |///|
		|/////|                    |///|
		--------------------------------
		      ^                    ^
			  |                    |
			 start                end
	*/
	//2nd situation
	if (end - start == 15) {
		int zero_start, zero_end;
		zero_start = ffs1(~bitmap & 0xFFFF);
		zero_end = fls1(~bitmap & 0xFFFF);
		start_port = zero_end;
		agg_num = (16 - (zero_end - zero_start + 1));
		agg_bitmap = ((1 << agg_num) - 1) & 0xFF;
		addr = ioport | (1 << 12) | (agg_bitmap << 4) | start_port;
	} else {//1st situation
		start_port = start - 1;
		agg_num = (end - start + 1);
		agg_bitmap = ((1 << agg_num) - 1) & 0xFF;
		addr = ioport | (1 << 12) | (agg_bitmap << 4) | start_port;
	}

	if (write && aggr_wr_size == 192) {
		loopback_log("%s do a joke in write change size from %d to", __func__, aggr_wr_size);
		aggr_wr_size = 72;
		loopback_log("%d\n", aggr_wr_size);
	}
	
	/*step 3: do read/write operation*/
	loopback_log("%s start port:%d (from bitmap:0x%x) port count:%d agg_bitmap:0x%x addr:0x%x buf_size:%d\n",
					write ? "W->" : "R<-", start_port, bitmap, agg_num, agg_bitmap, addr, write ? aggr_wr_size : aggr_rd_size - 4);

	ret = (write ? asr_sdio_writesb(addr, aggr_rd_buf, aggr_wr_size) : asr_sdio_readsb(addr, aggr_rd_buf, aggr_rd_size - 4));
	if (ret) {
		loopback_log("%s %s data failed\n", __func__, write ? "write" : "read");
		return ret;
	}
	
	if (write) {
		u32 *ptemp;
		if (is_loopback_display) {
			int i, temp_port;
			ptemp = (u32*)aggr_rd_buf;
			for (i = 0; i < agg_num; i++) {
				temp_port = (start_port + i) % 16;
				loopback_log("%s write port[%d] with size:%d: first:0x%x - last:0x%x\n",
						__func__, temp_port, aggr_port_size_print[temp_port], *ptemp, *(ptemp + aggr_port_size_print[temp_port] / 4 - 1));
				ptemp += (aggr_port_size_print[temp_port] / 4);
			}
		}
		asr_rtos_free(aggr_rd_buf);
		//kfree(aggr_wr_buf);
		aggr_wr_size = 0;
	} else {
		u32 *ptemp;
		//add fake 0x12345678 at the rx_buf's tail
		ptemp = ((u32*)aggr_rd_buf) + (aggr_rd_size / 4 - 1);
		*ptemp = 0x12345678;

		if (is_loopback_display) {
			int i, temp_port;
			ptemp = (u32*)aggr_rd_buf;
			for (i = 0; i < agg_num; i++) {
				temp_port = (start_port + i) % 16;
				loopback_log("%s read port[%d] with size:%d: first:0x%x - last:0x%x\n",
						__func__, temp_port, aggr_port_size_print[temp_port], *ptemp, *(ptemp + aggr_port_size_print[temp_port] / 4 - 1));
				ptemp += (aggr_port_size_print[temp_port] / 4);
			}

		}
	}
	return ret;
}

/*
	aggr_port_test: do aggr mode test
*/
static int aggr_port_tests(asr_drv_sdio_t *func)
{
	int block_size_arr[] = {16, 32, 256};
	int loop, test_times, ret;
	int aggr_block_size;

	for (loop = 0; loop < ARRAY_SIZE(block_size_arr); loop++) {
		if (direct_block_size)
			aggr_block_size = direct_block_size;
		else
			aggr_block_size = block_size_arr[loop];

        g_block_size = aggr_block_size;
		ret = asr_sdio_set_block_size(aggr_block_size);
		if (ret) {
			loopback_log("set block size[%d] failed", aggr_block_size);
			return ret;
		}

		loopback_log("set block_size: %d", aggr_block_size);
		for (test_times = 0; test_times < 16; test_times++) {
			ret = read_write_agg_port_test(func, aggr_block_size, 0);
			if (ret) {
				loopback_log("aggr mode read failed");
				return ret;
			}
			ret = read_write_agg_port_test(func, aggr_block_size, 1);
			if (ret) {
				loopback_log("aggr mode write failed");
				return ret;
			}

			loopback_log("cycle: %d\r\n", test_times);
		}
	}

	loopback_log("*******************************************\r\n"
	             "*********** aggrigation port test done! ********\r\n"
	             "*******************************************\r\n");

	return 0;
}

void sdio_get_irq_status_task()
{
    uint8_t reg_val[4];
    int ret = -1;
    uint32_t rcv_flags;

    while (1)
    {
        asr_rtos_wait_for_event(&sdio_loopback_event, SDIO_LOOPBACK_EVENT_INT, &rcv_flags, ASR_WAIT_FOREVER);
        asr_rtos_clear_event(&sdio_loopback_event, rcv_flags);

	    gpio_irq_times++;
        ret = sdio_readb_cmd52(HOST_INT_STATUS, reg_val);
	    if (ret == false)
        {
		    continue;
        }

	    if (reg_val[0] & HOST_INT_UPLD_ST) {
		    asr_rtos_lock_mutex(&upld_mutex);
		    reg_val[0] &= ~HOST_INT_UPLD_ST;

		    sdio_writeb_cmd52(HOST_INT_STATUS, reg_val[0]);
            if (start_flag == 1)
            {
		        asr_rtos_set_semaphore(&upld_completion);
            }
		    asr_rtos_unlock_mutex(&upld_mutex);
	    }
	    if (reg_val[0] & HOST_INT_DNLD_ST) {
		    asr_rtos_lock_mutex(&dnld_mutex);
		    reg_val[0] &= ~HOST_INT_DNLD_ST;

		    sdio_writeb_cmd52(HOST_INT_STATUS, reg_val[0]);
	        if (start_flag == 1)
	        {
		        asr_rtos_set_semaphore(&dnld_completion);
	        }
		    asr_rtos_unlock_mutex(&dnld_mutex);
	    }
    }
}

void sdio_get_irq_status_task_init()
{
    OSStatus ret;

    asr_rtos_create_event(&sdio_loopback_event);

    ret = asr_rtos_create_thread(&Irq_status_Task_Handler, ASR_OS_PRIORITY_HIGH,"irq_task", sdio_get_irq_status_task, 2048, 0);
    if (ret == kGeneralErr) {
    	loopback_log("create get irq status thread failed\r\n");
        return;
    }
}

void sdio_loopback_init()
{
    asr_rtos_init_mutex(&dnld_mutex);
    asr_rtos_init_mutex(&upld_mutex);

    asr_rtos_init_semaphore(&upld_completion, 0);
    asr_rtos_init_semaphore(&dnld_completion, 0);

    sdio_get_irq_status_task_init();
}

/*
	sdio_loopback_test: execute loopback test case
*/
int sdio_loopback_test()
{
	int ret;
	int i;
    asr_drv_sdio_t *func = NULL;

	trigger_slaver_do_rxtest(func);

	i = 0;
	while (i < loopback_times) {
		printf("####################### loopback_times: %d, i %d\n", loopback_times, i);
		/*single port test*/
		if (loopback_test_mode & 0x1) {
			ret = single_port_tests(func);
			if (ret < 0)
				return ret;
		}
		/*multi port test*/
		if (loopback_test_mode & 0x2) {
			ret = multi_port_tests(func);
			if (ret < 0)
				return ret;
		}
		/*aggregation mode test*/
		if (loopback_test_mode & 0x4) {
			ret = aggr_port_tests(func);
			if (ret < 0)
				return ret;
		}
		i++;
	}

	return 0;
}

/*
	asr_sdio_function_irq: isr in data1 if exist
*/
int asr_sdio_function_irq()
{
    asr_rtos_set_event(&sdio_loopback_event, SDIO_LOOPBACK_EVENT_INT);

	return 1;
}

#endif

