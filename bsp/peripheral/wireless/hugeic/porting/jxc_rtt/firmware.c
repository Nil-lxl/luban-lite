#include "hgic.h"
#include "txw901_v2.h"
#include "os_porting.h"

int request_firmware(struct firmware **fw, const char *name, void *dev)
{
    struct firmware *firmware = NULL;
    int ret  = 0;

    firmware = MALLOC(sizeof(struct firmware));
    if (firmware == NULL) {
        printf("%s:Malloc firmware failed!\n",__FUNCTION__);
        ret = -12;
        goto __failed;
    }
    memset(firmware, 0, sizeof(struct firmware));

    firmware->data = (unsigned char *)&txw901_v2;
    firmware->size = sizeof(txw901_v2);
    printf("Request firmware success,size:%d\n",firmware->size);
    *fw = firmware;
    return 0;

__failed:
    release_firmware((struct firmware *)firmware);
    return ret;
}

void release_firmware(struct firmware *fw)
{
    struct firmware *fwdata = (struct firmware *)fw;
    if(fwdata == NULL) {
        return;
    }
    FREE(fwdata);
}



