#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/jiffies.h>
#include <linux/handlers.h>
#include <linux/workqueue.h>
#include <linux/skbuff.h>
#include <net/netlink.h>

//#include "lib/umac2/umac.h"
#include "utils/includes.h"
#include "utils/common.h"
#include "utils/eloop.h"
#include "drivers/driver.h"
#include "wpa_supplicant_i.h"
#include "config.h"
#include "bss.h"
#include "common/wpa_common.h"
#include "scan.h"
#include "ctrl_iface.h"
#include "rsn_supp/wpa.h"
#include "hgloop.h"

static void wpa_supplicant_ctrl_iface_receive(int sock, void *eloop_ctx, void *sock_ctx)
{
    struct wpa_supplicant *wpa_s = eloop_ctx;
    struct ctrl_iface_priv *priv = sock_ctx;
    struct nl_msg *msg, *resp;
    char *reply = NULL, *reply_buf = NULL;
    size_t reply_len = 0;

    msg = nl_socket_recv(sock, 100);
    if (msg == NULL) {
        wpa_printf(MSG_ERROR, "rx ctrl msg failed!\r\n");
        return;
    }

    reply_buf = wpa_supplicant_ctrl_iface_process(wpa_s, (char *)msg->data, &reply_len);
    reply = reply_buf;

    if (!reply && reply_len == 1) {
        reply = "FAIL\n";
        reply_len = 5;
    } else if (!reply && reply_len == 2) {
        reply = "OK\n";
        reply_len = 3;
    }

    if (reply) {
        resp = nlmsg_new(reply_len, 0);
        if (resp) {
            memcpy(resp->data, reply_buf, reply_len);
            skb_put(resp, reply_len);
            //PRINTF("%s:%d:Send msg:%p\n",__FUNCTION__,__LINE__,resp);
            hgloop_send(priv->reply_sock, resp);
        }
    }
    os_free(reply_buf);
    kfree_skb(msg);
}

struct ctrl_iface_priv *wpa_supplicant_ctrl_iface_init(struct wpa_supplicant *wpa_s)
{
    struct ctrl_iface_priv *priv = os_zalloc(sizeof(*priv));
    if (priv == NULL) {
        return NULL;
    }

    priv->wpa_s = wpa_s;
    priv->sock = (int)nl_socket_alloc(wpa_s->ifname);
    priv->reply_sock = (int)nl_socket_alloc(wpa_s->ifname);
    mutex_init(&priv->lock);
    ASSERT(priv->sock && priv->reply_sock);
    eloop_register_read_sock(priv->sock, wpa_supplicant_ctrl_iface_receive, wpa_s, priv);
    return priv;
}

void wpa_supplicant_ctrl_iface_deinit(struct wpa_supplicant *wpa_s,
				      struct ctrl_iface_priv *priv)
{
    if (priv) {
        if (priv->sock) {
            eloop_unregister_read_sock(priv->sock);
            nl_socket_free((struct nl_sock *)priv->sock);
            priv->sock = 0;
        }
        if (priv->reply_sock) {
            nl_socket_free((struct nl_sock *)priv->reply_sock);
            priv->reply_sock = 0;
        }
        mutex_destroy(&priv->lock);
        os_free(priv);
    } else {
        PRINTF("%s:Error!input priv is NULL!\n",__FUNCTION__);
    }
}

