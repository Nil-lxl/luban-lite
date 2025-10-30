/*
 * netlink/handlers.c	default netlink message handlers
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2006 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_HANDLERS_H_
#define NETLINK_HANDLERS_H_

#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/msg_queue.h>
#include "utils/list.h"
#include <linux/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef nl_msg
#define nl_msg sk_buff
#endif

struct nl_cb;
struct nl_msg;
struct ucred;

struct sock;
struct net{
	struct sock		*genl_sock;
};

struct nl_sock
{
    struct dl_list    eloop_list;
    struct msg_queue  msgq;
    struct semaphore  done;
    uint32            ifindex;
    int32             err;
    atomic_t          event;
    void (*handler)(int sock, void *eloop_ctx, void *sock_ctx);
    void *eloop_data;
    void *user_data;
    uint32            magic;
};
#define nl_handle nl_sock
#define NL_SOCKET_MAGIC (0x2A2A2A2A)

typedef void (*eloop_sock_handler)(int sock, void *eloop_ctx, void *sock_ctx);

/**
 * @name Callback Typedefs
 * @{
 */

/**
 * nl_recvmsgs() callback for message processing customization
 * @ingroup cb
 * @arg msg		netlink message being processed
 * @arg arg		argument passwd on through caller
 */
typedef int (*nl_recvmsg_msg_cb_t)(struct nl_msg *msg, void *arg);

/** @} */

/**
 * Callback actions
 * @ingroup cb
 */
enum nl_cb_action {
	/** Proceed with wathever would come next */
	NL_OK,
	/** Skip this message */
	NL_SKIP,
	/** Stop parsing altogether and discard remaining messages */
	NL_STOP,
};

/**
 * Callback kinds
 * @ingroup cb
 */
enum nl_cb_kind {
	/** Default handlers (quiet) */
	NL_CB_DEFAULT,
	/** Verbose default handlers (error messages printed) */
	NL_CB_VERBOSE,
	/** Debug handlers for debugging */
	NL_CB_DEBUG,
	/** Customized handler specified by the user */
	NL_CB_CUSTOM,
	__NL_CB_KIND_MAX,
};

#define NL_CB_KIND_MAX (__NL_CB_KIND_MAX - 1)

/**
 * Callback types
 * @ingroup cb
 */
enum nl_cb_type {
	/** Message is valid */
	NL_CB_VALID,
	__NL_CB_TYPE_MAX,
};

#define NL_CB_TYPE_MAX (__NL_CB_TYPE_MAX - 1)

struct nl_cb
{
	nl_recvmsg_msg_cb_t	cb_set[NL_CB_TYPE_MAX+1];
	void *			cb_args[NL_CB_TYPE_MAX+1];
};

#define nl80211_handle_alloc   nl_socket_alloc
#define nl80211_handle_destroy nl_socket_free
void nl_socket_free(struct nl_sock *handle);
struct nl_sock *nl_socket_alloc(char *ifname);
struct nl_msg *nl_socket_recv(int sock, int tmo);
int nl_send_auto_complete(struct nl_sock *sk, struct nl_msg *msg);
extern struct nl_cb *	nl_cb_alloc(enum nl_cb_kind);
extern struct nl_cb *	nl_cb_clone(struct nl_cb *);
extern struct nl_cb *	nl_cb_get(struct nl_cb *);
//extern void		nl_cb_put(struct nl_cb *);
#define nl_cb_put(cb) kfree(cb)

extern int  nl_cb_set(struct nl_cb *, enum nl_cb_type, enum nl_cb_kind,
		      nl_recvmsg_msg_cb_t, void *);

extern enum nl_cb_type nl_cb_active_type(struct nl_cb *cb);

int nl_recvmsgs(struct nl_sock *sk, struct nl_cb *cb);

int nl_socket_add_membership(struct nl_sock *sk, int group);

int nl_get_multicast_id(const char *group);

#ifdef __cplusplus
}
#endif

#endif
