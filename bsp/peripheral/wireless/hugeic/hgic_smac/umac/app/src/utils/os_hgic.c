/*
 * wpa_supplicant/hostapd / Empty OS specific functions
 * Copyright (c) 2005-2006, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * This file can be used as a starting point when adding a new OS target. The
 * functions here do not really work as-is since they are just empty or only
 * return an error value. os_internal.c can be used as another starting point
 * or reference since it has example implementation of many of these functions.
 */

#include "includes.h"
#include <linux/types.h>
#include <linux/jiffies.h>
//#include <time.h>
#include "os.h"

//void os_sleep_ms(int msec);

int os_gmtime(os_time_t t, struct os_tm *tm)
{
    struct tm *local;
    time_t nowtime;

    local = localtime(&nowtime);
    tm->sec = local->tm_sec;
    tm->min = local->tm_min;
    tm->hour = local->tm_hour;
    tm->day = local->tm_mday;
    tm->month = local->tm_mon;
    tm->year = local->tm_year;
    return 0;
}

int os_get_time(struct os_time *t)
{
    uint32_t tick = jiffies;
    t->sec  = tick / 1000;
    t->usec = (tick % 1000) * 1000;
    return 0;
}


int os_get_reltime(struct os_reltime *t)
{
    struct timespec tm;

    get_sys_time(&tm);
    t->sec = tm.tv_sec;
    t->usec = tm.tv_nsec / 1000;
    return 0;
}


int os_mktime(int year, int month, int day, int hour, int min, int sec,
              os_time_t *t)
{
    return -1;
}

int os_daemonize(const char *pid_file)
{
    return -1;
}


void os_daemonize_terminate(const char *pid_file)
{
}


int os_get_random(unsigned char *buf, size_t len)
{
    int i = 0;
    
    srand(jiffies);
    for(i=0;i<len;i++){
        buf[i]=rand()&0xff;
    }
    return 0;
}


unsigned long os_random(void)
{
    return 0;
}


char *os_rel2abs_path(const char *rel_path)
{
    return NULL; /* strdup(rel_path) can be used here */
}


int os_program_init(void)
{
    return 0;
}


void os_program_deinit(void)
{
}


int os_setenv(const char *name, const char *value, int overwrite)
{
    return -1;
}


int os_unsetenv(const char *name)
{
    return -1;
}


char *os_readfile(const char *name, size_t *len)
{
    return NULL;
}


int os_fdatasync(FILE *stream)
{
    return 0;
}


//extern void memleak_check(unsigned int addr,int len);

size_t os_strlcpy(char *dest, const char *src, size_t size)
{
    const char *s = src;
    size_t left = size;

//    memleak_check(dest,size);

    if (left) {
        /* Copy string up to the maximum size of the dest buffer */
        while (--left != 0) {
            if ((*dest++ = *s++) == '\0') {
                break;
            }
        }
    }

    if (left == 0) {
        /* Not enough room for the string; force NUL-termination */
        if (size != 0) {
            *dest = '\0';
        }

        while (*s++); /* determine total src string length */
    }

    return s - src - 1;
}


int os_memcmp_const(const void *a, const void *b, size_t len)
{
    const char *aa = a;
    const char *bb = b;
    size_t i;
    char res;

    for (res = 0, i = 0; i < len; i++) {
        res |= aa[i] ^ bb[i];
    }
    return res;
}

int os_exec(const char *program, const char *arg, int wait_completion)
{
    return -1;
}

void sys_sleep(os_time_t sec, os_time_t usec)
{
    os_sleep_ms(sec*1000 + usec/1000);
}

void * os_memdup(const void *src, size_t len)
{
	void *r = MALLOC(len);

	if (r && src)
		memcpy(r, src, len);
	return r;
}

