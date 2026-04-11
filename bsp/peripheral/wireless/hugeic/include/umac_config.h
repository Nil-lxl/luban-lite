#ifndef _HGIC_UMAC_CONFIG_H_
#define _HGIC_UMAC_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif


struct umac_config {
    unsigned char  hg0[1024];
    unsigned char  hg1[1024];
};

struct umac_config *umac_configs(void);
extern int sys_save_umaccfg(struct umac_config *cfg);

#ifdef __cplusplus
}
#endif

#endif

