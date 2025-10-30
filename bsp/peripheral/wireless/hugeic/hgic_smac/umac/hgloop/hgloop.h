#ifndef _HUGE_IC_LOOP_H_
#define _HUGE_IC_LOOP_H_
int hgloop_init(void);
int hgloop_exit(void);
void hgloop_start(void);
struct sk_buff *hgloop_read(int sock, int tmo);
int hgloop_send(int sock, struct sk_buff *msg);
int hgloop_socket_alloc(void);
void hgloop_socket_free(int sock);
int hgloop_register_sock(int sock,
                         void (*handler)(int sock, void *loop_ctx, void *sock_ctx),
                         void *loop_ctx, void *sock_ctx);
void hgloop_unregister_read_sock(int sock);

#endif
