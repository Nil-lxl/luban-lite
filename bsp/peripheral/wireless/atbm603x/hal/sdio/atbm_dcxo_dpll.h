#include "atbm_hal.h"
int atbm_config_dpll(struct atbmwifi_common *hw_priv,char* value,int prjType,int dpllClock);
int atbm_config_dcxo(struct atbmwifi_common *hw_priv,char *value,int prjType,int dcxoType,int dpllClock);
int atbm_wait_wlan_rdy(struct atbmwifi_common *hw_priv);
int atbm_system_done(struct atbmwifi_common *hw_priv);
int atbm_config_jtag_mode(struct atbmwifi_common *hw_priv);
void atbm_set_config_to_smu(struct atbmwifi_common *hw_priv,int dpllClock);
void atbm_set_config_to_smu_apolloB(struct atbmwifi_common *hw_priv,int dpllClock);
