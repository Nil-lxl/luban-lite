#ifndef _HW_PORTING_SDIO_H_
#define _HW_PORTING_SDIO_H_

#define SDIO_CAP_IRQ(func) 1

typedef void sdio_driver_t;
//typedef void sdio_func_t;
//typedef void sdio_device_id_t;
typedef struct rt_sdio_device_id sdio_device_id_t;
typedef struct rt_sdio_function sdio_func_t;
typedef void (sdio_irq_handler_t)(struct sdio_func *);


//Copy from porting SDK
typedef struct mmc_cmd {
	unsigned int  opcode;
	unsigned int  arg;
	unsigned int  resp[4];
	unsigned int  flags;
/*rsponse types
 *bits:0~3
 */
#define RESP_MASK	(0xF)
#define RESP_NONE	(0)
#define RESP_R1		(1 << 0)
#define RESP_R1B	(2 << 0)
#define RESP_R2		(3 << 0)
#define RESP_R3		(4 << 0)
#define RESP_R4		(5 << 0)
#define RESP_R6		(6 << 0)
#define RESP_R7		(7 << 0)
#define RESP_R5		(8 << 0)	/*SDIO command response type*/
/*command types
 *bits:4~5
 */
#define CMD_MASK	(3 << 4)		/* command type */
#define CMD_AC		(0 << 4)
#define CMD_ADTC	(1 << 4)
#define CMD_BC		(2 << 4)
#define CMD_BCR		(3 << 4)

#define resp_type(cmd)	((cmd)->flags & RESP_MASK)

/*spi rsponse types
 *bits:6~8
 */
#define RESP_SPI_MASK	(0x7 << 6)
#define RESP_SPI_R1	(1 << 6)
#define RESP_SPI_R1B	(2 << 6)
#define RESP_SPI_R2	(3 << 6)
#define RESP_SPI_R3	(4 << 6)
#define RESP_SPI_R4	(5 << 6)
#define RESP_SPI_R5	(6 << 6)
#define RESP_SPI_R7	(7 << 6)

#define spi_resp_type(cmd)	((cmd)->flags & RESP_SPI_MASK)
/*
 * These are the command types.
 */
#define cmd_type(cmd)	((cmd)->flags & CMD_MASK)

	unsigned int  retries;	/* max number of retries */
	unsigned int  err;

	void *data;
	void *mrq;		/* associated request */
}mmc_command_t;

/*command */
#define	MMC_GO_IDLE_STATE	0
#define	MMC_SEND_OP_COND	1
#define	MMC_ALL_SEND_CID	2
#define	MMC_SET_RELATIVE_ADDR	3
#define	SD_SEND_RELATIVE_ADDR	3
#define	MMC_SET_DSR		4
#define	MMC_SLEEP_AWAKE		5
#define	MMC_SWITCH_FUNC		6
#define	 MMC_SWITCH_FUNC_CMDS	 0
#define	 MMC_SWITCH_FUNC_SET	 1
#define	 MMC_SWITCH_FUNC_CLR	 2
#define	 MMC_SWITCH_FUNC_WR	 3
#define	MMC_SELECT_CARD		7
#define	MMC_DESELECT_CARD	7
#define	MMC_SEND_EXT_CSD	8
#define	SD_SEND_IF_COND		8
#define	MMC_SEND_CSD		9
#define	MMC_SEND_CID		10
#define	IO_SEND_OP_COND		5
#define SD_IO_SEND_OP_COND      5
#define	SD_IO_RW_DIRECT		52
#define	SD_IO_RW_EXTENDED	53
#define MMC_SPI_CRC_ON_OFF      59

//RESP define
#define MMC_RSP_NONE        RESP_NONE
#define MMC_RSP_SPI_R1      RESP_SPI_R1
#define MMC_RSP_SPI_R4      RESP_SPI_R4
#define MMC_RSP_SPI_R5      RESP_SPI_R5
#define MMC_RSP_SPI_R7      RESP_SPI_R7
#define MMC_RSP_R1          RESP_R1
#define MMC_RSP_R4          RESP_R4
#define MMC_RSP_R5          RESP_R5
#define MMC_RSP_R6          RESP_R6
#define MMC_RSP_R7          RESP_R7
#define MMC_CMD_AC          CMD_AC
#define MMC_CMD_BC          CMD_BC
#define MMC_CMD_BCR         CMD_BCR

#define R5_ERROR			(1 << 11)
#define R5_FUNCTION_NUMBER	(1 << 9)
#define R5_OUT_OF_RANGE		(1 << 8)
#define R5_STATUS(x)		(x & 0xCB00)
#define R5_IO_CURRENT_STATE(x)	((x & 0x3000) >> 12)

//reg

#define SDIO_SPEED_EHS          0x02    /* Enable High-Speed mode */
#define SDIO_CCCR_CCCR		    0x00
#define SDIO_CCCR_IOEx          0x02//SDIO_REG_CCCR_IO_EN
#define SDIO_CCCR_IORx          0x03//SDIO_REG_CCCR_IO_RDY
#define SDIO_CCCR_IENx          0x04//SDIO_REG_CCCR_INT_EN        /* Function/Master Interrupt Enable */
#define SDIO_CCCR_INTx          0x05//SDIO_REG_CCCR_INT_PEND      /* Function Interrupt Pending */
#define SDIO_CCCR_ABORT         0x06//SDIO_REG_CCCR_IO_ABORT      /* function abort/card reset */
#define SDIO_CCCR_IF            0x07//SDIO_REG_CCCR_BUS_IF        /* bus interface controls */
#define SDIO_CCCR_CIS		    0x09                              /* common CIS pointer (3 bytes) */
#define SDIO_CCCR_SPEED         0x13//SDIO_CCCR_SPEED

#define SDIO_BUS_CD_DISABLE     0x80
#define SDIO_BUS_WIDTH_4BIT     0x02
#define SDIO_BUS_WIDTH_MASK     0x03
#define MMC_CARD_BUSY           0x80000000

#define MMC_BUS_WIDTH_1		0
#define MMC_BUS_WIDTH_4		2
#define MMC_BUS_WIDTH_8		3

#define MMC_TIMING_LEGACY	0
#define MMC_TIMING_MMC_HS	1
#define MMC_TIMING_SD_HS	2

#define SDIO_CAP_POLL(func)     (0)
#define HOST_SPI_CRC(host, crc)

#define CARD_BUSY	0x80000000	/* Card Power up status bit */

#define hgic_card_disable_cd(func)     (1)
#define hgic_card_set_highspeed(func)  (1)
#define hgic_host_is_spi(func)         (0)
#define hgic_card_cccr_widebus(func)   (0)
#define hgic_card_cccr_highspeed(func) (1)
#define hgic_host_highspeed(func)      (1)
#define hgic_host_supp_4bit(func)      (1)
#define hgic_card_highspeed(func)      (1)
#define hgic_card_max_clock(func)      hgic_sdio_get_max_clock((func))

extern unsigned long hgic_sdio_get_max_clock(sdio_func_t * func);
extern long hgic_mmc_send_cmd(sdio_func_t * func, void * cmd, int retries);
extern void hgic_mmc_set_clock(sdio_func_t * func, unsigned int hz);
extern void hgic_mmc_set_timing(sdio_func_t * func, unsigned int timing);
extern void hgic_mmc_set_bus_width(sdio_func_t * func, unsigned int width);
//extern long hgic_mmc_io_rw_direct(void *card, int write,unsigned fn, unsigned addr, unsigned char in, unsigned char *out);

unsigned char sys_sdio_readb(sdio_func_t *func, unsigned long addr, int *err);
unsigned char sys_sdio_writeb(sdio_func_t *func, unsigned char b, unsigned long addr, int *err);
long sys_sdio_memcpy_fromio(sdio_func_t *func, unsigned char *dest, unsigned long addr, int count);
long sys_sdio_memcpy_toio(sdio_func_t *func, unsigned long addr, unsigned char *src, int count);
void sys_sdio_claim_host(sdio_func_t *func);
void sys_sdio_release_host(sdio_func_t *func);
long sys_sdio_set_block_size(sdio_func_t *func, int bsize);
long sys_sdio_enable_func(sdio_func_t *func);
long sys_sdio_disable_func(sdio_func_t *func);
long sys_sdio_claim_irq(sdio_func_t *func, sdio_irq_handler_t irqhdl);
long sys_sdio_release_irq(sdio_func_t *func);
void *sys_sdio_get_drvdata(sdio_func_t *func);
long sys_sdio_set_drvdata(sdio_func_t *func, void *data);
long sys_sdio_register_driver(sdio_driver_t *driver);
void sys_sdio_unregister_driver(sdio_driver_t *driver);
unsigned long sys_sdio_get_func_num(sdio_func_t * func);
unsigned short sys_sdio_get_device_id(sdio_device_id_t *id);
unsigned short sys_sdio_get_vendor_id(sdio_device_id_t *id);

#define sdio_readb              sys_sdio_readb
#define sdio_writeb             sys_sdio_writeb
#define sdio_memcpy_fromio      sys_sdio_memcpy_fromio
#define sdio_memcpy_toio        sys_sdio_memcpy_toio
#define sdio_claim_host         sys_sdio_claim_host
#define sdio_release_host       sys_sdio_release_host
//#define sdio_set_block_size     sys_sdio_set_block_size
//#define sdio_enable_func        sys_sdio_enable_func
//#define sdio_disable_func       sys_sdio_disable_func
#define sdio_claim_irq          sys_sdio_claim_irq
#define sdio_release_irq        sys_sdio_release_irq
//#define sdio_get_drvdata        sys_sdio_get_drvdata
//#define sdio_set_drvdata        sys_sdio_set_drvdata
//#define sdio_register_driver    sys_sdio_register_driver
//#define sdio_unregister_driver  sys_sdio_unregister_driver

#define FUNC_NUM(f)             sys_sdio_get_func_num(f)
#define DEVID_ID(id)            sys_sdio_get_device_id(id)
#define DEVID_VENDOR(id)        sys_sdio_get_vendor_id(id)
#define FUNC_DEV(f)             sys_sdio_get_func_dev(f)

#endif

