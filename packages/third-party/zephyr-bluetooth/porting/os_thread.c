#include <aic_common.h>
#include <aic_core.h>
#include <zephyr/kernel.h>
#include "os_thread.h"

int RT_ThreadCreate(RT_Thread_t *thread, const char *name,
                    RT_ThreadEntry_t entry, void *arg,
                    uint8_t priority, uint32_t stackSize)
{
	thread->handle = rt_thread_create(name, entry, arg, stackSize,
	                                  priority, 5);

	pr_debug("%s(), name \"%s\", priority %d, stackSize %u, handle %p\n",
	       __func__, name, priority, (unsigned int)stackSize, thread->handle);

	if (thread->handle == NULL) {
		pr_err("%s failed!\n", __func__);
		return -RT_ERROR;
	} else {
		rt_thread_startup(thread->handle);
	}

	return RT_EOK;
}
