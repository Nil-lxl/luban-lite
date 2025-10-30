#include "utils/includes.h"
#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/list.h>
#include <linux/rculist.h>
#include <linux/workqueue.h>
#include <linux/netdevice.h>
#include <linux/handlers.h>
#include <linux/semaphore.h>
#include <net/netlink.h>

#include "umac_config.h"
#include "utils/common.h"
#include "fst/fst.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"
#include "p2p_supplicant.h"
#include "config.h"
#include "common/wpa_common.h"
#include "utils/eloop.h"
#include "hgloop.h"
#include "scan.h"
#include "bss.h"
#include "ap.h"
#include "crypto/sha1.h"
#if defined(__MBED__) || defined(__CSKY__)
#include "lib/lmac/hgic.h"
#include "lib/umac2/umac.h"
#else
#include "hgic.h"
#endif

#define WPA_SUPPLICANT_CLEANUP_INTERVAL 10
#define osWaitForever                   (0xffffffffu)

struct wpa_ctrl {
#ifdef CONFIG_CTRL_IFACE_UDP
	int s;
#ifdef CONFIG_CTRL_IFACE_UDP_IPV6
	struct sockaddr_in6 local;
	struct sockaddr_in6 dest;
#else /* CONFIG_CTRL_IFACE_UDP_IPV6 */
	struct sockaddr_in local;
	struct sockaddr_in dest;
#endif /* CONFIG_CTRL_IFACE_UDP_IPV6 */
	char *cookie;
	char *remote_ifname;
	char *remote_ip;
#endif /* CONFIG_CTRL_IFACE_UDP */
#ifdef CONFIG_CTRL_IFACE_UNIX
	int s;
	struct sockaddr_un local;
	struct sockaddr_un dest;
#endif /* CONFIG_CTRL_IFACE_UNIX */
#ifdef CONFIG_CTRL_IFACE_NAMED_PIPE
	HANDLE pipe;
#endif /* CONFIG_CTRL_IFACE_NAMED_PIPE */
};

extern int wpa_debug_level;
extern int wpa_debug_show_keys;
extern int wpa_debug_timestamp;

void wpas_ctrl_iface_msg_cb(void *ctx, int level,
                            enum wpa_msg_type type,
                            const char *txt, size_t len);

struct wpas_mgr {
    struct wpa_ctrl   ctrl;
    struct wpa_global wpas_global;
    struct semaphore  wpas_cp;
    struct msg_queue  ctrl_recvq;
    struct mutex      lock;
    unsigned int open;
} _wpas_mgr;

static int wpas_netdev_notify(struct notifier_block *nb, unsigned long state, void *ndev)
{
    int ret = -1;
    struct net_device *dev = (struct net_device *)ndev;
    size_t reply_len = 0;
    char *reply = NULL;
    char *reply_buf = NULL;

    if(dev == NULL || dev->wpas == NULL) {
        PRINTF("%s,%d:Input param error!\n",__FUNCTION__,__LINE__);
        return -EINVAL;
    }

    switch (state) {
        case NETDEV_UP:
            //ret = wpas_cli(dev->name, "DRIVER_EVENT INTERFACE_ENABLED", NULL, 0);
            reply_buf = wpa_supplicant_ctrl_iface_process(dev->wpas, "DRIVER_EVENT INTERFACE_ENABLED",
                            &reply_len);
            break;
        case NETDEV_DOWN:
            //ret = wpas_cli(dev->name, "DRIVER_EVENT INTERFACE_DISABLED", NULL, 0);
            reply_buf = wpa_supplicant_ctrl_iface_process(dev->wpas, "DRIVER_EVENT INTERFACE_DISABLED",
                            &reply_len);
            break;
        default:
            break;
    }
    reply = reply_buf;
    if (!reply && reply_len == 1) {
        ret = -1;
    } else if (!reply && reply_len == 2) {
        ret = 0;
    }
    if(reply_buf) {
        os_free(reply_buf);
    }
    return ret;
}

static const struct notifier_block wpas_netdev_notifier = {
    .notifier_call = wpas_netdev_notify,
};

/* Periodic cleanup tasks */
static void wpas_periodic(void *eloop_ctx, void *timeout_ctx)
{
    struct wpa_global *global = eloop_ctx;
    struct wpa_supplicant *wpa_s;

    eloop_register_timeout(WPA_SUPPLICANT_CLEANUP_INTERVAL, 0,
                           wpas_periodic, global, NULL);
#ifdef CONFIG_P2P
    if (global->p2p) {
        p2p_expire_peers(global->p2p);
    }
#endif /* CONFIG_P2P */
    for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next) {
        wpa_bss_flush_by_age(wpa_s, wpa_s->conf->bss_expiration_age);
#ifdef CONFIG_AP
        ap_periodic(wpa_s);
#endif /* CONFIG_AP */
    }
}

int wpas_init(void)
{
    int ret = 0;

    PRINTF("wpas_init\n");
    _wpas_mgr.wpas_global.drv_count = 1;
    sema_init(&_wpas_mgr.wpas_cp, 0);
    mutex_init(&_wpas_mgr.lock);
    dl_list_init(&_wpas_mgr.wpas_global.p2p_srv_bonjour);
    dl_list_init(&_wpas_mgr.wpas_global.p2p_srv_upnp);
    register_netdevice_notifier(&wpas_netdev_notifier);
    _wpas_mgr.wpas_global.drv_priv = os_calloc(_wpas_mgr.wpas_global.drv_count, sizeof(void *));

    msg_queue_init(&_wpas_mgr.ctrl_recvq, 128);
    wpa_msg_register_cb(wpas_ctrl_iface_msg_cb);

    ret = eap_register_methods();//hostapd also
    if (ret) {
        wpa_printf(MSG_ERROR, "Failed to register EAP methods");
        if (ret == -2) {
            wpa_printf(MSG_ERROR, "Two or more EAP methods used the same EAP type.");
        }
        return ret;
    }

    if (fst_global_init()) {
        wpa_printf(MSG_ERROR, "Failed to initialize FST");
        return -1;
    }

#ifdef CONFIG_WIFI_DISPLAY
    if (wifi_display_init(&_wpas_mgr.wpas_global) < 0) {
        wpa_printf(MSG_ERROR, "Failed to initialize Wi-Fi Display");
        ASSERT(0);
        return -1;
    }
#endif /* CONFIG_WIFI_DISPLAY */

    eloop_register_timeout(WPA_SUPPLICANT_CLEANUP_INTERVAL, 0,
                           wpas_periodic, &_wpas_mgr.wpas_global, NULL);
    return 0;
}

void _wpas_start(void *ifname, void *data)
{
    struct wpa_interface iface;
    struct net_device *ndev = NULL;

    mutex_lock(&_wpas_mgr.lock);
    ndev = net_device_get_by_name((char *)ifname);
    if (ndev && !ndev->wpas) {
        PRINTF("_wpas_start enter\r\n");
        //net_device_open(ndev);
        memset(&iface, 0, sizeof(struct wpa_interface));
        iface.driver = "nl80211";
        iface.ifname = ifname;
        iface.confname = ifname;
        //iface.p2p_mgmt = 1;
        ndev->wpas = wpa_supplicant_add_iface(&_wpas_mgr.wpas_global, &iface, NULL);
        PRINTF("_wpas_start leave, %s\r\n", ndev->wpas ? "OK" : "Fail");
        net_device_open(ndev);
    }
    sema_up(&_wpas_mgr.wpas_cp);
    mutex_unlock(&_wpas_mgr.lock);
}

void _wpas_stop(void *ifname, void *data)
{
    struct net_device *ndev = NULL;

    mutex_lock(&_wpas_mgr.lock);
    ndev = net_device_get_by_name((char *)ifname);
    if (ndev && ndev->wpas) {
        PRINTF("wpas_stop enter\r\n");
        net_device_close(ndev);
        wpa_supplicant_remove_iface(&_wpas_mgr.wpas_global, (struct wpa_supplicant *)ndev->wpas, 0);
        //net_device_close(ndev);
        ndev->wpas = NULL;
        PRINTF("wpas_stop leave\r\n");
    }
    sema_up(&_wpas_mgr.wpas_cp);
    mutex_unlock(&_wpas_mgr.lock);
}

int wpas_start(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev) {
        //mutex_lock(&_wpas_mgr.lock);
        eloop_register_timeout(0, 0, _wpas_start, ifname, 0);
        sema_down(&_wpas_mgr.wpas_cp);
        //mutex_unlock(&_wpas_mgr.lock);
        return ndev->wpas ? 0 : -1;
    }
    return -1;
}

int wpas_stop(char *ifname)
{
    struct net_device *ndev = net_device_get_by_name(ifname);
    if (ndev) {
        //mutex_lock(&_wpas_mgr.lock);
        hgics_wpacli_disable_network(ifname);
        hgics_wpacli_remove_network(ifname);
        eloop_register_timeout(0, 0, _wpas_stop, ifname, 0);
        sema_down(&_wpas_mgr.wpas_cp);
        //mutex_unlock(&_wpas_mgr.lock);
        return ndev->wpas ? -1 : 0;
    }
    return 0;
}

int wpas_cli(char *ifname, char *cmd, char *reply_buff, int reply_len)
{
    char *ptr;
    int   len = 0;
    int   cmd_len = 0;
    struct nl_msg *msg;
    struct nl_msg *reply;
    struct wpa_supplicant *wpas = NULL;
    struct net_device *ndev = NULL;

    mutex_lock(&_wpas_mgr.lock);
    ndev = net_device_get_by_name(ifname);
    if (ndev == NULL) {
        PRINTF("%s:%d:Can not find netif name:%s\n", __FUNCTION__, __LINE__, ifname);
        mutex_unlock(&_wpas_mgr.lock);
        return -EINVAL;
    }
    wpas = ndev->wpas;
    if (wpas == NULL || wpas->ctrl_iface == NULL) {
        PRINTF("%s:%d:wpa_supplicant already stop,skip cmd:%s\n", __FUNCTION__, __LINE__, cmd);
        mutex_unlock(&_wpas_mgr.lock);
        return -ENODEV;
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
        wpa_printf(MSG_DEBUG, "wpa_cli cmd:[%s]\r\n", msg->data);
        PRINTF("%s:%d:Send msg:%s\n", __FUNCTION__, __LINE__, msg->data);
        mutex_lock(&wpas->ctrl_iface->lock);
        hgloop_send(wpas->ctrl_iface->sock, msg);
        reply = hgloop_read(wpas->ctrl_iface->reply_sock, 1000);
        mutex_unlock(&wpas->ctrl_iface->lock);
        if (reply) {
            len = reply->len < reply_len ? reply->len : reply_len;
            if (reply_buff) {
                strncpy(reply_buff, (char *)reply->data, len);
            }
            PRINTF("wpa_cli Reply:    %s\r\n", reply->data);
            wpa_printf(MSG_DEBUG, "    %s\r\n", reply->data);
            kfree_skb(reply);
        }
    }
    mutex_unlock(&_wpas_mgr.lock);
    return len;
}

int wpas_passphrase(char *ssid, char *passphrase, char psk[32])
{
    return pbkdf2_sha1(passphrase, (u8 *) ssid, os_strlen(ssid), 4096, (u8 *)psk, 32);
}

struct wpa_ctrl *wpa_ctrl_open(const char *ctrl_path)
{
    PRINTF("%s,%d,path:%s\n", __FUNCTION__, __LINE__, ctrl_path);
    _wpas_mgr.open = 1;
    return (void *)&_wpas_mgr;
}

void wpa_ctrl_close(struct wpa_ctrl *ctrl)
{
    mutex_lock(&_wpas_mgr.lock);
    _wpas_mgr.open = 0;
    mutex_unlock(&_wpas_mgr.lock);
    PRINTF("%s,%d!\n", __FUNCTION__, __LINE__);
}

int wpa_ctrl_attach(struct wpa_ctrl *ctrl)
{
    PRINTF("%s,%d!\n", __FUNCTION__, __LINE__);
}

int wpa_ctrl_detach(struct wpa_ctrl *ctrl)
{
    PRINTF("%s,%d!\n", __FUNCTION__, __LINE__);
}

int wpa_ctrl_get_fd(struct wpa_ctrl *ctrl)
{
    return (int *)&_wpas_mgr;
}

int wpa_ctrl_recv(struct wpa_ctrl *ctrl, char *reply, size_t *reply_len)
{
    struct sk_buff *msg = NULL;
    size_t buff_len = *reply_len;

    msg = (struct sk_buff *)msg_queue_get(&_wpas_mgr.ctrl_recvq, osWaitForever);
    if (msg == NULL || buff_len == 0) {
        PRINTF("Recv msg error,return!\n");
        return;
    }
    if (reply) {
        MEMCPY(reply, msg->data, msg->len > buff_len ? buff_len : msg->len);
    }
    kfree_skb(msg);
    return 0;
}

int wpa_ctrl_request(struct wpa_ctrl *ctrl, const char *cmd, size_t cmd_len,
                     char *reply, size_t *reply_len,
                     void (*msg_cb)(char *msg, size_t len))
{
#if 0
    struct timeval tv;
    struct os_reltime started_at;
    int res;
    fd_set rfds;
    const char *_cmd;
    char *cmd_buf = NULL;
    size_t _cmd_len;

#ifdef CONFIG_CTRL_IFACE_UDP
    if (ctrl->cookie) {
        char *pos;
        _cmd_len = os_strlen(ctrl->cookie) + 1 + cmd_len;
        cmd_buf = os_malloc(_cmd_len);
        if (cmd_buf == NULL)
        { return -1; }
        _cmd = cmd_buf;
        pos = cmd_buf;
        os_strlcpy(pos, ctrl->cookie, _cmd_len);
        pos += os_strlen(ctrl->cookie);
        *pos++ = ' ';
        os_memcpy(pos, cmd, cmd_len);
    } else
#endif /* CONFIG_CTRL_IFACE_UDP */
    {
        _cmd = cmd;
        _cmd_len = cmd_len;
    }

    errno = 0;
    started_at.sec = 0;
    started_at.usec = 0;
retry_send:
    if (send(ctrl->s, _cmd, _cmd_len, 0) < 0) {
        if (errno == EAGAIN || errno == EBUSY || errno == EWOULDBLOCK) {
            /*
             * Must be a non-blocking socket... Try for a bit
             * longer before giving up.
             */
            if (started_at.sec == 0)
            { os_get_reltime(&started_at); }
            else {
                struct os_reltime n;
                os_get_reltime(&n);
                /* Try for a few seconds. */
                if (os_reltime_expired(&n, &started_at, 5))
                { goto send_err; }
            }
            os_sleep(1, 0);
            goto retry_send;
        }
send_err:
        os_free(cmd_buf);
        return -1;
    }
    os_free(cmd_buf);

    for (;;) {
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        FD_ZERO(&rfds);
        FD_SET(ctrl->s, &rfds);
        res = select(ctrl->s + 1, &rfds, NULL, NULL, &tv);
        if (res < 0 && errno == EINTR)
        { continue; }
        if (res < 0)
        { return res; }
        if (FD_ISSET(ctrl->s, &rfds)) {
            res = recv(ctrl->s, reply, *reply_len, 0);
            if (res < 0)
            { return res; }
            if ((res > 0 && reply[0] == '<') ||
                (res > 6 && strncmp(reply, "IFNAME=", 7) == 0)) {
                /* This is an unsolicited message from
                 * wpa_supplicant, not the reply to the
                 * request. Use msg_cb to report this to the
                 * caller. */
                if (msg_cb) {
                    /* Make sure the message is nul
                     * terminated. */
                    if ((size_t) res == *reply_len)
                    { res = (*reply_len) - 1; }
                    reply[res] = '\0';
                    msg_cb(reply, res);
                }
                continue;
            }
            *reply_len = res;
            break;
        } else {
            return -2;
        }
    }
    return 0;
#endif
    int len = 0;
    len = wpas_cli(hgic_param_ifname(NULL), cmd, reply, reply_len);
    if (len <= 0) {
        PRINTF("%s:Cmd:%s failed\n", __FUNCTION__, cmd);
        *reply_len = 0;
        return -1;
    }
    if ((len > 0 && reply[0] == '<') || (len > 6 && strncmp(reply, "IFNAME=", 7) == 0)) {
        /* This is an unsolicited message from
         * wpa_supplicant, not the reply to the
         * request. Use msg_cb to report this to the
         * caller. */
        if (msg_cb) {
            /* Make sure the message is nul
             * terminated. */
            if ((size_t) len == *reply_len)
            { len = (*reply_len) - 1; }
            reply[len] = '\0';
            msg_cb(reply, len);
        }
    }
    *reply_len = len;
    return 0;
}


extern int wpa_debug_level;

void wpas_ctrl_iface_msg_cb(void *ctx, int level,
                            enum wpa_msg_type type,
                            const char *txt, size_t len)
{
    struct sk_buff *skb = NULL;
    char levelstr[64] = {0};
    int llen = 0;
    int ret = 0;

    if (level < wpa_debug_level) {
        PRINTF("msg level:%d under debug level %d,return!\n", level, wpa_debug_level);
        return;
    }

    if(_wpas_mgr.open) {
        os_snprintf(levelstr, sizeof(levelstr), "<%d>", level);//all
        llen = os_strlen(levelstr);
        skb = alloc_skb(llen + len + 4);
        if (skb == NULL) {
            PRINTF("Error,no skb,size:%d\n", llen + len);
            return;
        }
        memset(skb->data, 0, llen + len);
        os_memcpy(skb->data, levelstr, llen);
        os_memcpy(skb->data + llen, txt, len);
        skb_put(skb, llen + len);

        ret = msg_queue_put(&_wpas_mgr.ctrl_recvq, skb, osWaitForever);
        if (ret) {
            PRINTF("%s,Error send msgqueue,ret%d\n", __FUNCTION__,ret);
        }
    } else {
        //PRINTF("wpa_debug:%s\n",txt);
    }
}

int wpa_cli_main(int argc, char *argv[])
{
    ;
}

