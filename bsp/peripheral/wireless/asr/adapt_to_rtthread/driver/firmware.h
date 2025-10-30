#ifndef __FIRMWARE_H__
#define __FIRMWARE_H__

#include <stdint.h>
#include <stdio.h>

enum asr_patch {
    ASR_BOOTLOADER,
    ASR_FIRMWARE,
    ASR_SDD_BIN,
    ASR_ETF_FW,
};

struct asr_firmware {
    unsigned int size;
    unsigned int *data;
};

extern int request_firmware(struct asr_firmware *fw, enum asr_patch patch);
extern void release_firmware(struct asr_firmware *fw);

#endif /* __FIRMWARE_H__ */
