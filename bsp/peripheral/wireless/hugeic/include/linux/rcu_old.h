#ifndef _LINUX_RCU_H_
#define _LINUX_RCU_H_

#include <linux/defs.h>
#include <linux/types.h>

#if 1
struct rcu_head 
{
    unsigned int reserve; 
};

#define rcu_access_pointer(p) (p)
#define rcu_dereference(p) (p)
#define rcu_dereference_protected(p,c) (p)
#define rcu_assign_pointer(p,v) (p=v)
#define RCU_INIT_POINTER(p,v)(p=v)
#define synchronize_rcu()
#define call_rcu(r,f) f(r)
#define synchronize_net()
#define rcu_dereference_raw(p) (p)
#define rcu_dereference_check(p, a) (p)
#define rtnl_dereference(p) (p)
#define rcu_read_lock_held() (1)
#define mb() //__DMB()
#define barrier() //__DMB()
#define rcu_barrier() //__DMB()
#define smp_mb()


/*
#define rcu_dereference_check(p, a) ({\
        if(!a) {\
            PRINTF("Warning:suspicious rcu_dereference_check() usage\n");\
        }\
        p;\
})
*/
void _rcu_read_lock(void);
void _rcu_read_unlock(void);
#if 1
#define rcu_init()        _rcu_init();
#define rcu_read_lock()   _rcu_read_lock()
#define rcu_read_unlock() _rcu_read_unlock()
#else
#define rcu_init()
#define rcu_read_lock()
#define rcu_read_unlock()
#endif

#else 

struct rcu_head {
    struct rcu_head *next;
	void (*func)(struct rcu_head *head);
};

#define mb()          //__DMB()
#define barrier()     //__DMB()
#define rcu_barrier() //__DMB()
#define smp_mb()      //__DMB()
#define smp_wmb()     //__DMB()
#define smp_read_barrier_depends()  //__DMB()

#define ACCESS_ONCE(x)     (*(volatile typeof(x) *)&(x))

//#define RCU_INIT_POINTER(p, v) \
//                    do { \
//                        p = (typeof(*v) __force __rcu *)(v); \
//                    } while (0)

#define RCU_INIT_POINTER(p,v)(p=v)

#define __rcu_access_pointer(p, space) \
                ({ \
                    typeof(*p) *_________p1 = (typeof(*p)*__force )ACCESS_ONCE(p); \
                    ((typeof(*p) __force __kernel *)(_________p1)); \
                })

#define rcu_dereference_check(p, space) \
            ({ \
                typeof(*p) *_________p1 = (typeof(*p)*__force )ACCESS_ONCE(p); \
                smp_read_barrier_depends(); \
                ((typeof(*p) __force __kernel *)(_________p1)); \
            })

#define __rcu_dereference_protected(p, c, space) \
            ({ \
                ((typeof(*p) __force __kernel *)(p)); \
            })

#define __rcu_assign_pointer(p, v, space) \
        do { \
            smp_wmb(); \
            (p) = (typeof(*v) __force space *)(v); \
        } while (0)

#define rcu_dereference(p) rcu_dereference_check(p, 0)

#define rcu_dereference_protected(p, c) __rcu_dereference_protected((p), (c), __rcu)

#define rcu_assign_pointer(p, v) __rcu_assign_pointer((p), (v), __rcu)

#define rcu_dereference_raw(p)   rcu_dereference_check(p, 1)

#define rcu_access_pointer(p) __rcu_access_pointer((p), __rcu)

#define rtnl_dereference(p)	rcu_dereference_protected(p, lockdep_rtnl_is_held())

#define synchronize_net() synchronize_rcu()

#define rcu_read_lock_held() (1)

void rcu_init(void);
void synchronize_rcu(void);
void rcu_read_lock(void);
void rcu_read_unlock(void);
void call_rcu(struct rcu_head *head, void (*func)(struct rcu_head *rcu));

#define __is_kfree_rcu_offset(offset) ((offset) < 4096)

static inline void kfree_call_rcu(struct rcu_head *head,
                  void (*func)(struct rcu_head *rcu))
{
    call_rcu(head, func);
}

#define __kfree_rcu(head, offset) \
        do { \
            kfree_call_rcu(head, (void (*)(struct rcu_head *))(unsigned long)(offset)); \
        } while (0)

#define kfree_rcu(ptr, rcu_head)					\
	__kfree_rcu(&((ptr)->rcu_head), offsetof(typeof(*(ptr)), rcu_head))


#endif

#endif
