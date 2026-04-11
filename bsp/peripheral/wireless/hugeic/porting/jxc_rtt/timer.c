#include "os_porting.h"

typedef void (*timer_func_t)(unsigned long data);
static void os_timer_callback(void *priv);

typedef void (*timer_func_t)(unsigned long data);
struct hgic_timer_param {
    rt_timer_t timer;
    timer_func_t cb;
    void *timer_param;
};

int os_timer_create(uintptr_t *tmr,void *param, timer_func_t func,int ms)
{
    struct hgic_timer_param *timer = NULL;
    static unsigned char index = 0;
    char timer_name[16] = {0};
    unsigned int timeout_ms;
    int ret = 0;

    snprintf(timer_name, sizeof(timer_name), "hgt_%d", index);
    index ++;

    timer = MALLOC(sizeof(struct hgic_timer_param));
    if(timer == NULL) {
        hgic_err("Error,no memory!\n");
        return -1;
    }
    rt_memset(timer,0,sizeof(struct hgic_timer_param));
    timer->timer_param = param;
    timer->cb          = func;
    timeout_ms = ms > 10 ? ms : 10;

    timer->timer = rt_timer_create(timer_name, os_timer_callback, (void *)timer,
                        rt_tick_from_millisecond(timeout_ms),
                        RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER | RT_TIMER_FLAG_DEACTIVATED);
                        //not around & not auto timer
    if(timer->timer == NULL) {
        hgic_err("Create timer failed,ret:%d\n",ret);
        return ret;
    }

    if(tmr) {
        *tmr = (uintptr_t)timer->timer;
    }
    return 0;
}


void os_timer_stop(uintptr_t timer)
{
    rt_timer_t tmr = (rt_timer_t)timer;
    if(tmr == NULL) {
        return;
    }
    rt_timer_stop(tmr);
}

void os_timer_start(uintptr_t timer)
{
    rt_timer_t tmr = (rt_timer_t)timer;
    if(tmr == NULL) {
        return;
    }
    rt_timer_start(tmr);
}

static void os_timer_callback(void *priv)
{
    struct hgic_timer_param *timer = (struct hgic_timer_param *)priv;
    if(timer == NULL) {
        hgic_err("Input param error\n");
        return;
    }
    timer->cb(timer->timer_param);
}

int os_timer_change(uintptr_t timer,unsigned int ms)
{
    rt_timer_t tmr = (rt_timer_t)timer;
    unsigned int msec = ms > 10 ? ms : 10;
    if(tmr == NULL) {
        hgic_err("Input param error!\n");
        return -1;
    }
    return rt_timer_control(tmr,  RT_TIMER_CTRL_SET_TIME, &msec);
}

void os_timer_free(uintptr_t timer)
{
    rt_timer_t tmr = (rt_timer_t)timer;
    if(tmr == NULL) {
        hgic_err("Input param error!\n");
        return -1;
    }
    rt_timer_delete(tmr);
}

