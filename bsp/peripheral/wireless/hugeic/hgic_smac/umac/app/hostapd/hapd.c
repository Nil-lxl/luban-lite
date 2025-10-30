#include "utils/includes.h"
#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/list.h>
#include <linux/rculist.h>
#include <linux/workqueue.h>
#include <linux/netdevice.h>
#include <linux/handlers.h>
#include <net/netlink.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>

#include "utils/includes.h"

#include "utils/common.h"
#include "utils/eloop.h"
#include "utils/uuid.h"
#include "crypto/random.h"
#include "crypto/tls.h"
#include "common/version.h"
#include "drivers/driver.h"
#include "eap_server/eap.h"
#include "eap_server/tncs.h"
#include "ap/hostapd.h"
#include "ap/ap_config.h"
#include "ap/ap_drv_ops.h"
#include "fst/fst.h"
#include "config_file.h"
#include "eap_register.h"
#include "ctrl_iface.h"
#include "utils/eloop.h"
#include "hgloop.h"
#if defined(__MBED__) || defined(__CSKY__)
#include "lib/lmac/hgic.h"
#include "lib/umac2/umac.h"
#else
#include "hgic.h"
#endif

struct hapd_data {
    struct hapd_interfaces interfaces;
    int reply_sock;
    struct mutex lock;
};

struct hapd_global {
    void  *drv_priv;
    size_t drv_count;
};
struct {
    struct hapd_global global;
    struct semaphore   cp;
    struct mutex      lock;
} hapd_mgr;

static int hapd_netdev_notify(struct notifier_block *nb, unsigned long state, void *ndev)
{
    int ret = 0;
    struct net_device *dev = (struct net_device *)ndev;
    char *reply_buff = NULL;
    const int reply_size = 4096;
    int reply_len = 0;

    if(dev == NULL || dev->hostapd == NULL) {
        PRINTF("%s,%d:Input param error!\n",__FUNCTION__,__LINE__);
        return -EINVAL;
    }

    reply_buff = MALLOC(reply_size);
    if(reply_buff == NULL) {
        PRINTF("%s,%d:Error,no memory!\n",__FUNCTION__,__LINE__);
        return -ENOMEM;
    }
    switch (state) {
        case NETDEV_UP:
            //ret = hapd_cli(dev->name, "INTERFACE_ENABLED", NULL, 0);
            reply_len = hostapd_ctrl_iface_receive_process(dev->hostapd, "INTERFACE_ENABLED",
                reply_buff, reply_size);
            break;
        case NETDEV_GOING_DOWN:
            //ret = hapd_cli(dev->name, "INTERFACE_DISABLED", NULL, 0);
            reply_len = hostapd_ctrl_iface_receive_process(dev->hostapd, "INTERFACE_DISABLED",
                reply_buff, reply_size);
            break;
        default:
            break;
    }
    if (reply_len <= 0) {
        ret = -EINPROGRESS;
    }
    if(reply_buff) {
        FREE(reply_buff);
    }
    return ret;
}

static const struct notifier_block hapd_netdev_notifier = {
    .notifier_call = hapd_netdev_notify,
};


/**
 * hostapd_driver_init - Preparate driver interface
 */
static int hostapd_driver_init(struct hostapd_iface *iface)
{
    struct wpa_init_params params;
    struct hostapd_data *hapd = iface->bss[0];
    struct hostapd_bss_config *conf = hapd->conf;
    u8 *b = conf->bssid;
    struct wpa_driver_capa capa;

    if (hapd->driver == NULL || hapd->driver->hapd_init == NULL) {
        wpa_printf(MSG_ERROR, "No hostapd driver wrapper available");
        return -1;
    }

    /* Initialize the driver interface */
    if (!(b[0] | b[1] | b[2] | b[3] | b[4] | b[5])) {
        b = NULL;
    }

    memset(&params, 0, sizeof(params));

    if (hapd_mgr.global.drv_priv == NULL &&  hapd->driver->global_init) {
        hapd_mgr.global.drv_priv = hapd->driver->global_init(iface->interfaces, hapd->conf->iface);
        if (hapd_mgr.global.drv_priv == NULL) {
            wpa_printf(MSG_ERROR, "Failed to initialize driver '%s'", hapd->driver->name);
            return -1;
        }
    }

    params.global_priv = hapd_mgr.global.drv_priv;
    params.bssid = b;
    params.ifname = hapd->conf->iface;
    params.driver_params = hapd->iconf->driver_params;
    params.use_pae_group_addr = hapd->conf->use_pae_group_addr;
    params.own_addr = hapd->own_addr;

    hapd->drv_priv = hapd->driver->hapd_init(hapd, &params);
    if (hapd->drv_priv == NULL) {
        wpa_printf(MSG_ERROR, "%s driver initialization failed.", hapd->driver->name);
        hapd->driver = NULL;
        return -1;
    }

    if (hapd->driver->get_capa && hapd->driver->get_capa(hapd->drv_priv, &capa) == 0) {
        struct wowlan_triggers *triggs;

        iface->drv_flags = capa.flags;
		iface->drv_flags2 = capa.flags2;
        iface->probe_resp_offloads = capa.probe_resp_offloads;

        /*
         * Use default extended capa values from per-radio information
         */
        iface->extended_capa = capa.extended_capa;
        iface->extended_capa_mask = capa.extended_capa_mask;
        iface->extended_capa_len = capa.extended_capa_len;
        iface->drv_max_acl_mac_addrs = capa.max_acl_mac_addrs;

        /*
         * Override extended capa with per-interface type (AP), if
         * available from the driver.
         */
        hostapd_get_ext_capa(iface);

        triggs = wpa_get_wowlan_triggers(conf->wowlan_triggers, &capa);
        if (triggs && hapd->driver->set_wowlan) {
            if (hapd->driver->set_wowlan(hapd->drv_priv, triggs)) {
                wpa_printf(MSG_ERROR, "set_wowlan failed");
            }
        }
        FREE(triggs);
    }

    return 0;
}


/**
 * hostapd_interface_init - Read configuration file and init BSS data
 *
 * This function is used to parse configuration file for a full interface (one
 * or more BSSes sharing the same radio) and allocate memory for the BSS
 * interfaces. No actiual driver operations are started.
 */
static struct hostapd_iface *
hostapd_interface_init(struct hapd_interfaces *interfaces, const char *if_name,
                       const char *config_fname, int debug)
{
    struct hostapd_iface *iface;
    int k;

    wpa_printf(MSG_ERROR, "Configuration file: %s", config_fname);
    iface = hostapd_init(interfaces, config_fname);
    if (!iface) {
        return NULL;
    }

    if (if_name) {
        os_strlcpy(iface->conf->bss[0]->iface, if_name, sizeof(iface->conf->bss[0]->iface));
        os_strlcpy(iface->bss[0]->ifname,if_name,sizeof(iface->bss[0]->ifname));
        //PRINTF("%s,%d:hostapd data ifname set to %s\n",__FUNCTION__,__LINE__,
        //    iface->bss[0]->ifname);
    }
    iface->interfaces = interfaces;
    return iface;
}

static int hostapd_global_init(void)
{
    memset(&hapd_mgr.global, 0, sizeof(hapd_mgr.global));
    hapd_mgr.global.drv_count = 1;
    if (eap_server_register_methods()) {
        wpa_printf(MSG_ERROR, "Failed to register EAP methods");
        return -1;
    }
    return 0;
}

static void hostapd_global_deinit(void)
{
    eap_server_unregister_methods();
}

static int hostapd_global_run(struct hapd_interfaces *ifaces, int daemonize,
                              const char *pid_file)
{
#ifdef EAP_SERVER_TNC
    int tnc = 0;
    size_t i, k;

    for (i = 0; !tnc && i < ifaces->count; i++) {
        for (k = 0; k < ifaces->iface[i]->num_bss; k++) {
            if (ifaces->iface[i]->bss[0]->conf->tnc) {
                tnc++;
                break;
            }
        }
    }

    if (tnc && tncs_global_init() < 0) {
        wpa_printf(MSG_ERROR, "Failed to initialize TNCS");
        return -1;
    }
#endif /* EAP_SERVER_TNC */
    return 0;
}


#ifdef CONFIG_WPS
static int gen_uuid(const char *txt_addr)
{
    u8 addr[ETH_ALEN];
    u8 uuid[UUID_LEN];
    char buf[100];

    if (hwaddr_aton(txt_addr, addr) < 0) {
        return -1;
    }

    uuid_gen_mac_addr(addr, uuid);
    if (uuid_bin2str(uuid, buf, sizeof(buf)) < 0) {
        return -1;
    }

    PRINTF("%s\n", buf);

    return 0;
}
#endif /* CONFIG_WPS */


#ifndef HOSTAPD_CLEANUP_INTERVAL
#define HOSTAPD_CLEANUP_INTERVAL 10
#endif /* HOSTAPD_CLEANUP_INTERVAL */

static int hostapd_periodic_call(struct hostapd_iface *iface, void *ctx)
{
    hostapd_periodic_iface(iface);
    return 0;
}


/* Periodic cleanup tasks */
static void hostapd_periodic(void *eloop_ctx, void *timeout_ctx)
{
    struct hapd_interfaces *interfaces = eloop_ctx;
    eloop_register_timeout(HOSTAPD_CLEANUP_INTERVAL, 0, hostapd_periodic, interfaces, NULL);
    hostapd_for_each_interface(interfaces, hostapd_periodic_call, NULL);
}

void hapd_cli_reply(struct hapd_interfaces *interfaces, char *reply, int reply_len)
{
    struct nl_msg *resp = NULL;
    struct hapd_data *hapd = container_of(interfaces, struct hapd_data, interfaces);

    if (reply_len <= 0) {
        reply = "FAIL\n";
        reply_len = 5;
    }
    resp = nlmsg_new(reply_len + 4, 0);
    if (resp) {
        memcpy(resp->data, reply, reply_len);
        skb_put(resp, reply_len);
        hgloop_send(hapd->reply_sock, resp);
    }
}

int hapd_cli(char *ifname, char *cmd, char *reply_buff, int reply_len)
{
    char *ptr;
    int   len = 0;
    int   cmd_len = 0;
    struct nl_msg *msg;
    struct nl_msg *reply;
    struct net_device *ndev = NULL;
    struct hapd_data  *hapd;

    mutex_lock(&hapd_mgr.lock);
    ndev = net_device_get_by_name(ifname);
    if (ndev == NULL || ndev->hostapd == NULL) {
        PRINTF("%s,%d:Can not find netif name :%s!\n",__FUNCTION__,__LINE__,ifname);
        mutex_unlock(&hapd_mgr.lock);
        return -EINVAL;
    }

    hapd = (struct hapd_data *)ndev->hostapd;
    if (hapd == NULL || !hapd->interfaces.global_ctrl_sock) {
        PRINTF("%s,%d:Error!\n",__FUNCTION__,__LINE__);
        mutex_unlock(&hapd_mgr.lock);
        return -EBADFD;
    }

    cmd_len = strlen(cmd);
    msg = (struct nl_msg *)nlmsg_new(cmd_len + 2, 0);
    if (msg) {
        strcpy((char *)msg->data, cmd);
        msg->data[cmd_len] = 0;
        skb_put(msg, cmd_len);

        /*change cmd string to upper case.*/
        ptr = (char *)msg->data;
        while (*ptr && *ptr != ' ') {
            *ptr = (char)toupper(*ptr);
            ptr++;
        }
        wpa_printf(MSG_DEBUG, "hapd_cli cmd:[%s]\r\n", msg->data);
        mutex_lock(&hapd->lock);
        PRINTF("%s:%d:Send cmd:[%s]\r\n",__FUNCTION__,__LINE__,msg->data);
        hgloop_send(hapd->interfaces.global_ctrl_sock, msg);
        reply = hgloop_read(hapd->reply_sock, 1000);
        mutex_unlock(&hapd->lock);
        if (reply) {
            len = reply->len < reply_len ? reply->len : reply_len;
            if (reply_buff) {
                strncpy(reply_buff, (char *)reply->data, len);
            }
            wpa_printf(MSG_DEBUG, "    %s\r\n", reply->data);
            PRINTF("%s:Reply:%s\n",__FUNCTION__,reply_buff);
            kfree_skb(reply);
        }
    }
    mutex_unlock(&hapd_mgr.lock);
    return len;
}

void _hapd_start(void *if_name, void *data)
{
    char *ifname = (char *)if_name;
    struct net_device *ndev = NULL;
    struct hapd_data  *hapd = NULL;

    mutex_lock(&hapd_mgr.lock);
    ndev = net_device_get_by_name(ifname);
    if (ndev == NULL)
        goto __OUT;

    net_device_open(ndev);
    hapd = os_zalloc(sizeof(struct hapd_data));
    if (hapd == NULL)
        goto __OUT;

    //hapd->interfaces.reload_config = hostapd_reload_config;
    hapd->interfaces.config_read_cb = hostapd_config_read;
    hapd->interfaces.for_each_interface = hostapd_for_each_interface;
    //hapd->interfaces.ctrl_iface_init = hostapd_ctrl_iface_init;
    //hapd->interfaces.ctrl_iface_deinit = hostapd_ctrl_iface_deinit;
    hapd->interfaces.driver_init = hostapd_driver_init;
    hapd->interfaces.global_iface_path = NULL;
    hapd->interfaces.global_iface_name = NULL;
    hapd->interfaces.global_ctrl_sock = 0;
    hapd->interfaces.count = 1;
    dl_list_init(&hapd->interfaces.global_ctrl_dst);

    hapd->interfaces.iface = os_calloc(hapd->interfaces.count, sizeof(struct hostapd_iface *));
    if (hapd->interfaces.iface == NULL) {
        wpa_printf(MSG_ERROR, "malloc failed");
        FREE(hapd);
        goto __OUT;
    }

    if (fst_global_init()) {
        wpa_printf(MSG_ERROR, "Failed to initialize global FST context");
        FREE(hapd->interfaces.iface);
        FREE(hapd);
        goto __OUT;
    }

#if defined(CONFIG_FST) && defined(CONFIG_CTRL_IFACE)
    if (!fst_global_add_ctrl(fst_ctrl_cli)) {
        wpa_printf(MSG_WARNING, "Failed to add CLI FST ctrl");
    }
#endif /* CONFIG_FST && CONFIG_CTRL_IFACE */

    hapd->interfaces.iface[0] = hostapd_interface_init(&hapd->interfaces, ifname, ifname, 0);
    if (!hapd->interfaces.iface[0]) {
        wpa_printf(MSG_ERROR, "Failed to initialize interface");
        FREE(hapd->interfaces.iface);
        FREE(hapd);
        goto __OUT;
    }

    if (hostapd_driver_init(hapd->interfaces.iface[0]) || hostapd_setup_interface(hapd->interfaces.iface[0])) {
        FREE(hapd->interfaces.iface);
        FREE(hapd);
        goto __OUT;
    }

    eloop_register_timeout(HOSTAPD_CLEANUP_INTERVAL, 0, hostapd_periodic, &hapd->interfaces, NULL);
    hostapd_global_ctrl_iface_init(&hapd->interfaces);
    hostapd_global_run(&hapd->interfaces, 0, NULL);
    mutex_init(&hapd->lock);
    hapd->reply_sock = (int)nl_socket_alloc(NULL);
    ASSERT(hapd->reply_sock);

    ndev->hostapd = hapd;
__OUT:
    sema_up(&hapd_mgr.cp);
    mutex_unlock(&hapd_mgr.lock);
}

void _hapd_stop(void *if_name, void *data)
{
    char *ifname = (char *)if_name;
    struct net_device *ndev = NULL;
    struct hapd_data  *hapd;

    mutex_lock(&hapd_mgr.lock);
    PRINTF("%s,%d:Enter!\n",__FUNCTION__,__LINE__);
    ndev = net_device_get_by_name(ifname);
    if (ndev) {
        hapd = ndev->hostapd;
        if (hapd) {
            nl_socket_free(hapd->reply_sock);
            //hostapd_interface_deinit_free(hapd->interfaces.iface[0]);
            hostapd_global_ctrl_iface_deinit(&hapd->interfaces);
            eloop_cancel_timeout(hostapd_periodic, &hapd->interfaces, NULL);
            hostapd_interface_deinit_free(hapd->interfaces.iface[0]);
            wpa_drivers[0]->global_deinit(hapd_mgr.global.drv_priv);
            //FREE(hapd_mgr.global.drv_priv);//nl   struct nl80211_global
            hapd_mgr.global.drv_priv = NULL;
#ifdef EAP_SERVER_TNC
            tncs_global_deinit();
#endif /* EAP_SERVER_TNC */
            fst_global_deinit();
            FREE(hapd->interfaces.iface);
            mutex_destroy(&hapd->lock);
            FREE(hapd);
        }
        ndev->hostapd = NULL;
        net_device_close(ndev);
    }
    PRINTF("%s,%d:Leave!\n",__FUNCTION__,__LINE__);
    sema_up(&hapd_mgr.cp);
    mutex_unlock(&hapd_mgr.lock);
}

int hapd_init(void)
{
    PRINTF("hapd_init,build time,%s,%s\n",__DATE__,__TIME__);
    hostapd_global_init();
    sema_init(&hapd_mgr.cp, 0);
    mutex_init(&hapd_mgr.lock);
    register_netdevice_notifier(&hapd_netdev_notifier);
    return 0;
}

int hapd_start(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev) {
        //mutex_lock(&hapd_mgr.lock);
        eloop_register_timeout(0, 0, _hapd_start, ifname, 0);
        sema_down(&hapd_mgr.cp);
        //mutex_unlock(&hapd_mgr.lock);
        return ndev->hostapd ? 0 : -1;
    }
    return -1;
}

int hapd_stop(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev) {
        //mutex_lock(&hapd_mgr.lock);
        eloop_register_timeout(0, 0, _hapd_stop, ifname, 0);
        sema_down(&hapd_mgr.cp);
        //mutex_unlock(&hapd_mgr.lock);
        return ndev->hostapd ? -1 : 0;
    }
    return 0;
}

int hostapd_main(int argc, char *argv[])
{
    ;
}

int hostapd_cli_main(int argc, char *argv[])
{
    ;
}



