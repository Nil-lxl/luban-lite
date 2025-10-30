//#include "typedef.h"
#include "firmware.h"
//#include "fccfs.h"
#include <stdlib.h>

//#define ASR5505_FIRMWARE_NAME   MAKEFOURCC('A', 'S', 'R', 'F')

__attribute__ ((aligned(4))) const uint8_t asr5xxx_wifi_fw_bin[] = {

#ifdef CONFIG_ASR5505
    #if SDIO_BLOCK_SIZE == 512
    #include "fmacfw_asr5505_blksz512.bin.inc"  
    #else
        #ifdef SDIO_LOOPBACK_TEST
            #include "fmacfw_asr5505_sdio_loopback.bin.inc"  
        #else
            #ifdef CFG_OOO_UPLOAD
            #include "fmacfw_asr5505_ooo.bin.inc"
            #else
            #include "fmacfw_asr5505.bin.inc"
            #endif
        #endif
    #endif
#endif


};
int request_firmware(struct asr_firmware *fw, enum asr_patch patch)
{
//    fw->data = 0;
//    fw->size = 0;
//
//    fw->data = burn_data;
//    fw->size = sizeof(burn_data);

#if 1
    switch (patch) {
    case ASR_BOOTLOADER:
        break;
    case ASR_FIRMWARE:
        fw->size = sizeof(asr5xxx_wifi_fw_bin);
        fw->data = (void *)asr5xxx_wifi_fw_bin;
 
        break;
    case ASR_SDD_BIN:
        break;
    case ASR_ETF_FW:
        break;
    default:
        break;
    }
#endif
    return 0;
}

void release_firmware(struct asr_firmware *fw)
{
    if (fw)
    {
        free(fw->data);
        fw->data = NULL;
    }
}
