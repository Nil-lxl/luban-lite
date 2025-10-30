#ifndef _LINUX_RCU_H_
#define _LINUX_RCU_H_

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

#define RCU_INIT_POINTER(p, v) \
                    do { \
                        p = (typeof(*v) __force __rcu *)(v); \
                    } while (0)

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

#define rcu_init()  _rcu_init() 

#define call_rcu(h,f)   _call_rcu((h), (f))

//void rcu_init(void);
void synchronize_rcu(void);
void rcu_read_lock(void);
void rcu_read_unlock(void);
//void call_rcu(struct rcu_head *head, void (*func)(struct rcu_head *rcu));

#endif
