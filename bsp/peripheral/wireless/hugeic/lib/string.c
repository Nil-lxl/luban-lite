#include <linux/types.h>
#include <linux/string.h>
#include <linux/mutex.h>
//#include <stdio.h>
#include <stdarg.h>

int scnprintf(char *buf, unsigned long size, const char *fmt, ...)
{
    va_list ap;
    int ret;

    if (!size) {
        return 0;
    }

    va_start(ap, fmt);
    ret = vsnprintf(buf, size, fmt, ap);
    va_end(ap);

    if (ret < 0) {
        return 0;
    }
    if ((unsigned long) ret >= size) {
        return size - 1;
    }
    return ret;
}

