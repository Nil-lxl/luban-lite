#ifndef _LINUX_MODULE_H_
#define _LINUX_MODULE_H_
#include <linux/moduleparam.h>

#define module_init(...)
#define module_exit(...)
#define module_param(...)
#define MODULE_LICENSE(...)
#define MODULE_DESCRIPTION(...)
#define MODULE_PARM_DESC(...)
#define MODULE_AUTHOR(...)
#define EXPORT_SYMBOL(...)

#define THIS_MODULE (0)
#define try_module_get(m) (1)
#define module_put(m) (1)
#define request_module(...)
#define subsys_initcall(...)
#define fs_initcall(...)
#define late_initcall(...)
#endif
