/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "fb_disp_info.h"
#include "tkc/mem.h"
#include "base/lcd.h"
#include "tkc/thread.h"
#include "awtk_global.h"
#include "tkc/time_now.h"
#include "tkc/mutex.h"
#include "tkc/semaphore.h"
#include "blend/image_g2d.h"
#include "base/system_info.h"
#include "lcd/lcd_mem_bgr565.h"
#include "lcd/lcd_mem_rgb565.h"
#include "lcd/lcd_mem_bgra8888.h"
#include "lcd/lcd_mem_rgba8888.h"
#include "lcd/lcd_mem_bgr888.h"
#include "lcd/lcd_mem_rgb888.h"
#include "base/lcd_orientation_helper.h"

// #define TK_USE_AIC_FPS

#ifdef WITH_AIC_G2D
#include <stdint.h>
#include "aic_g2d.h"
#include "aic_rtos_mem.h"
#include "rtthread.h"

static cma_buffer lcd_buffer[3];

#ifdef WITH_LCD_FLUSH_AND_SWAP
static rt_thread_t g_bswap = NULL;
static rt_sem_t g_sem_spare = NULL;
static rt_sem_t g_sem_ready = NULL;
static rt_mutex_t g_lck_fblist = NULL;
static bool_t g_app_quited = FALSE;
#endif /* WITH_LCD_FLUSH_AND_SWAP */
#endif /* WITH_AIC_G2D */

static fb_info_t g_fb;

#ifdef TK_USE_AIC_FPS
#define US_PER_SEC      1000000

static int draw_fps = 0;

static double get_time_gap(struct timeval *start, struct timeval *end)
{
    double diff;

    if (end->tv_usec < start->tv_usec) {
        diff = (double)(US_PER_SEC + end->tv_usec - start->tv_usec)/US_PER_SEC;
        diff += end->tv_sec - 1 - start->tv_sec;
    } else {
        diff = (double)(end->tv_usec - start->tv_usec)/US_PER_SEC;
        diff += end->tv_sec - start->tv_sec;
    }

    return diff;
}

static int cal_fps(double gap, int cnt)
{
    return (int)(cnt / gap);
}

static void cal_frame_rate()
{
    static int start_cal = 0;
    static int frame_cnt = 0;
    static struct timeval start, end;
    double interval = 0.5;
    double gap = 0;

    if (start_cal == 0) {
        start_cal = 1;
        start.tv_sec = 0;
        start.tv_usec = (long)aic_get_time_us();
    }

    end.tv_sec = 0;
    end.tv_usec = (long)aic_get_time_us();
    gap = get_time_gap(&start, &end);
    if (gap >= interval) {
        draw_fps = cal_fps(gap, frame_cnt);
        frame_cnt = 0;
        start_cal = 0;
    } else {
        frame_cnt++;
    }
    return;
}

int fbdev_draw_fps(void)
{
    return draw_fps;
}
#endif

static void on_app_exit(void) {
  fb_info_t* fb = &g_fb;

#ifdef WITH_AIC_G2D
#ifdef WITH_LCD_FLUSH_AND_SWAP
  g_app_quited = TRUE;
  rt_sem_release(g_sem_spare);
  rt_sem_release(g_sem_ready);
  sleep_ms(200);

  if (g_sem_spare) {
    rt_sem_delete(g_sem_spare);
  }

  if (g_sem_ready) {
    rt_sem_delete(g_sem_ready);
  }

  if (g_lck_fblist) {
    rt_mutex_delete(g_lck_fblist);
  }

  aic_cma_buf_del_ge(lcd_buffer[2].buf_head);
#endif

  tk_aic_g2d_close();
  aic_cma_buf_close();
#endif

  tk_fb_disp_exit(fb);

  log_info("on_app_exit\n");
}

static void aic_fb_enable(void)
{
#ifndef AIC_DISP_COLOR_BLOCK
  static bool first_frame = true;
  /* enable display power after flush first frame */
  if (first_frame) {
    if (g_fb.fb)
      mpp_fb_ioctl(g_fb.fb, AICFB_POWERON, 0);
    first_frame = false;
  }
#endif
}

static ret_t (*lcd_mem_rtos_flush_default)(lcd_t* lcd);
static ret_t lcd_mem_rtos_flush(lcd_t* lcd) {
  fb_info_t* fb = &g_fb;
  int dummy = 0;

  aic_fb_enable();

  mpp_fb_ioctl(fb->fb, AICFB_WAIT_FOR_VSYNC, &dummy);
  if (lcd_mem_rtos_flush_default) {
    lcd_mem_rtos_flush_default(lcd);
  }
#ifdef TK_USE_AIC_FPS
  cal_frame_rate();
#endif
  return RET_OK;
}

static ret_t lcd_swap_sync(lcd_t* lcd) {
  static int swap_lcd = 1;
  fb_info_t* fb = &g_fb;

  mpp_fb_ioctl(fb->fb, AICFB_PAN_DISPLAY, &swap_lcd);

  aic_fb_enable();

  mpp_fb_ioctl(fb->fb, AICFB_WAIT_FOR_VSYNC, 0);

  swap_lcd = swap_lcd == 1 ? 0 : 1;

  return RET_OK;
}

static void lcd_swap_fb(lcd_t* lcd) {
  lcd_mem_t* mem = (lcd_mem_t*)lcd;
  uint8_t* tmp_fb = mem->offline_fb;

  lcd_mem_set_offline_fb(mem, mem->online_fb);
  lcd_mem_set_online_fb(mem, tmp_fb);
}

static ret_t lcd_mem_rtos_swap(lcd_t* lcd) {
  lcd_mem_t* mem = (lcd_mem_t*)lcd;
  fb_info_t* fb = &g_fb;

  /* before hardware operations, it is necessary to clear the dcache to prevent CPU write backs. */
  aicos_dcache_clean_invalid_range((unsigned long *)mem->offline_fb, (unsigned long)fb_size(fb));
  aicos_dcache_clean_invalid_range((unsigned long *)mem->online_fb, (unsigned long)fb_size(fb));

  if (lcd_swap_sync(lcd) != RET_OK)
    log_error("lcd_swap_sync failed\n");

  lcd_swap_fb(lcd);
#ifdef TK_USE_AIC_FPS
  cal_frame_rate();
#endif
  return RET_OK;
}

static ret_t lcd_mem_init_drawing_fb(lcd_t* lcd, bitmap_t* fb) {
  lcd_mem_t* mem = (lcd_mem_t*)lcd;

  if (fb != NULL) {
    memset(fb, 0x00, sizeof(bitmap_t));

    fb->format = mem->format;
    fb->buffer = mem->offline_gb;
    fb->w = lcd_get_physical_width(lcd);
    fb->h = lcd_get_physical_height(lcd);
    graphic_buffer_attach(mem->offline_gb, mem->offline_fb, fb->w, fb->h);
    bitmap_set_line_length(fb, mem->line_length);
  }

  return RET_OK;
}

static bitmap_t* lcd_mem_init_online_fb(lcd_t* lcd, bitmap_t* fb, lcd_orientation_t o) {
  uint32_t w = 0;
  uint32_t h = 0;
  lcd_mem_t* mem = (lcd_mem_t*)lcd;
  uint32_t bpp = bitmap_get_bpp_of_format(BITMAP_FMT_BGR565);

  if (o == LCD_ORIENTATION_0 || o == LCD_ORIENTATION_180) {
    w = lcd_get_width(lcd);
    h = lcd_get_height(lcd);
  } else {
    log_debug("ldc swap ex mode did't support rotation\n");
  }

  memset(fb, 0x00, sizeof(bitmap_t));
  fb->w = w;
  fb->h = h;
  fb->buffer = mem->online_gb;
  fb->format = mem->format;
  graphic_buffer_attach(mem->online_gb, mem->online_fb, w, h);
  bitmap_set_line_length(fb, tk_max(fb->w * bpp, mem->online_line_length));

  return fb;
}

static ret_t lcd_mem_rtos_swap_ex(lcd_t* lcd) {
  bitmap_t online_fb;
  bitmap_t offline_fb;
  const dirty_rects_t* dirty_rects;
  lcd_mem_t* mem = (lcd_mem_t*)lcd;
  system_info_t* info = system_info();
  uint8_t* fb = lcd_mem_get_offline_fb(mem);
  lcd_orientation_t o = info->lcd_orientation;
  fb_info_t* fb_info = &g_fb;

  lcd_mem_init_drawing_fb(lcd, &offline_fb);
  lcd_mem_init_online_fb(lcd, &online_fb, o);

  if (mem->wait_vbi != NULL) {
    mem->wait_vbi(mem->wait_vbi_ctx);
  }

  aicos_dcache_clean_invalid_range((unsigned long *)mem->offline_fb, (unsigned long)fb_size(fb_info));
  aicos_dcache_clean_invalid_range((unsigned long *)mem->online_fb, (unsigned long)fb_size(fb_info));

  if (lcd_swap_sync(lcd) != RET_OK)
    log_error("lcd_swap_sync failed\n");

  dirty_rects = lcd_fb_dirty_rects_get_dirty_rects_by_fb(&(mem->fb_dirty_rects_list), fb);
  if (dirty_rects != NULL && dirty_rects->nr > 0) {
    if (dirty_rects->disable_multiple) {
      const rect_t* dr = (const rect_t*)&(dirty_rects->max);
      if (o == LCD_ORIENTATION_0) {
        image_copy(&online_fb, &offline_fb, dr, dr->x, dr->y);
      } else {
        image_rotate(&online_fb, &offline_fb, dr, o);
      }
    } else {
      uint32_t i = 0;
      for (i = 0; i < dirty_rects->nr; i++) {
        const rect_t* dr = (const rect_t*)dirty_rects->rects + i;
        if (o == LCD_ORIENTATION_0) {
          image_copy(&online_fb, &offline_fb, dr, dr->x, dr->y);
        } else {
          image_rotate(&online_fb, &offline_fb, dr, o);
        }
      }
    }
  }

  lcd_swap_fb(lcd);
#ifdef TK_USE_AIC_FPS
  cal_frame_rate();
#endif
  return RET_OK;
}

#ifdef WITH_LCD_FLUSH_AND_SWAP
#define FB_LIST_NUM 3

enum {
  FB_TAG_UND = 0,
  FB_TAG_SPARE,
  FB_TAG_READY,
  FB_TAG_BUSY
};

typedef struct fb_taged {
  int fbid;
  int tags;
} fb_taged_t;

static fb_taged_t s_fblist[FB_LIST_NUM];

static void init_fblist(int num)
{
  memset(s_fblist, 0, sizeof(s_fblist));
  for (int i = 0; i < num && i < FB_LIST_NUM; i++) {
    s_fblist[i].fbid = i;
    s_fblist[i].tags = FB_TAG_SPARE;
  }
}

static fb_taged_t* get_spare_fb()
{
  for (int i = 0; i < FB_LIST_NUM; i++) {
    if (s_fblist[i].tags == FB_TAG_SPARE) {
      return &s_fblist[i];
    }
  }
  return NULL;
}

static fb_taged_t* get_busy_fb()
{
  for (int i = 0; i < FB_LIST_NUM; i++) {
    if (s_fblist[i].tags == FB_TAG_BUSY) {
      return &s_fblist[i];
    }
  }
  return NULL;
}

inline static fb_taged_t* get_ready_fb()
{
  fb_taged_t* last_busy_fb = get_busy_fb();

  if (last_busy_fb) {
    /* get the first ready slot next to the busy one */
    for (int i = 1; i < FB_LIST_NUM; i++) {
      int next_ready_fbid = (last_busy_fb->fbid + i) % FB_LIST_NUM;

      if (s_fblist[next_ready_fbid].tags == FB_TAG_READY) {
        return &s_fblist[next_ready_fbid];
      }
    }
  } else {
    /* no last busy, get the first ready one */
    for (int i = 0; i < FB_LIST_NUM; i++) {
      if (s_fblist[i].tags == FB_TAG_READY) {
        return &s_fblist[i];
      }
    }
  }
  return NULL;
}

static ret_t lcd_rtos_init_drawing_fb(lcd_mem_t* mem, bitmap_t* fb) {
  return_value_if_fail(mem != NULL && fb != NULL, RET_BAD_PARAMS);

  memset(fb, 0x00, sizeof(bitmap_t));

  fb->w = lcd_get_physical_width((lcd_t*)mem);
  fb->h = lcd_get_physical_height((lcd_t*)mem);
  fb->format = mem->format;
  fb->buffer = mem->offline_gb;
  graphic_buffer_attach(mem->offline_gb, mem->offline_fb, fb->w, fb->h);
  bitmap_set_line_length(fb, mem->line_length);

  return RET_OK;
}

static ret_t lcd_rtos_init_online_fb(lcd_mem_t* mem, bitmap_t* fb, uint8_t* buff, uint32_t w, uint32_t h, uint32_t line_length) {
  return_value_if_fail(mem != NULL && fb != NULL && buff != NULL, RET_BAD_PARAMS);

  memset(fb, 0x00, sizeof(bitmap_t));

  fb->w = w;
  fb->h = h;
  fb->format = mem->format;
  fb->buffer = mem->online_gb;
  graphic_buffer_attach(mem->online_gb, buff, w, h);
  bitmap_set_line_length(fb, line_length);

  return RET_OK;
}

static ret_t lcd_rtos_flush(lcd_t* base, int fbid) {
  uint8_t* buff = NULL;
  fb_info_t* fb = &g_fb;
  int fb_nr = fb_number(fb);
  uint32_t size = fb_size(fb);
  lcd_mem_t* lcd = (lcd_mem_t*)base;
  const dirty_rects_t* dirty_rects = NULL;
  lcd_orientation_t o = system_info()->lcd_orientation;

  return_value_if_fail(lcd != NULL && fb != NULL && fbid < fb_nr, RET_BAD_PARAMS);

  buff = fb->fbmem0 + size * fbid;

  bitmap_t online_fb;
  bitmap_t offline_fb;
  lcd_rtos_init_drawing_fb(base, &offline_fb);
  lcd_rtos_init_online_fb(lcd, &online_fb, buff, fb_width(fb), fb_height(fb), fb_line_length(fb));

  lcd_fb_dirty_rects_add_fb_info(&(lcd->fb_dirty_rects_list), buff);
  lcd_fb_dirty_rects_update_all_fb_dirty_rects(&(lcd->fb_dirty_rects_list), base->dirty_rects);

  dirty_rects = lcd_fb_dirty_rects_get_dirty_rects_by_fb(&(lcd->fb_dirty_rects_list), buff);
  if (dirty_rects != NULL && dirty_rects->nr > 0) {
    for (int i = 0; i < dirty_rects->nr; i++) {
      const rect_t* dr = (const rect_t*)dirty_rects->rects + i;
#ifdef WITH_FAST_LCD_PORTRAIT
      if (system_info()->flags & SYSTEM_INFO_FLAG_FAST_LCD_PORTRAIT) {
        rect_t rr = lcd_orientation_rect_rotate_by_anticlockwise(dr, o, lcd_get_width(base), lcd_get_height(base));
        image_copy(&online_fb, &offline_fb, &rr, rr.x, rr.y);
      } else
#endif
      {
        if (o == LCD_ORIENTATION_0) {
          image_copy(&online_fb, &offline_fb, dr, dr->x, dr->y);
        } else {
          image_rotate(&online_fb, &offline_fb, dr, o);
        }
      }
    }
  }

  lcd_fb_dirty_rects_reset_dirty_rects_by_fb(&(lcd->fb_dirty_rects_list), buff);
}


static ret_t lcd_mem_rtos_write_buff(lcd_t* lcd) {
  ret_t ret = RET_OK;

  if (g_app_quited) {
    return ret;
  }

  if (lcd->draw_mode != LCD_DRAW_OFFLINE) {
    rt_sem_take(g_sem_spare, RT_WAITING_FOREVER);

    rt_mutex_take(g_lck_fblist, RT_WAITING_NO);
    fb_taged_t* spare_fb = get_spare_fb();
    assert(spare_fb);
    rt_mutex_release(g_lck_fblist);
    ret = lcd_rtos_flush(lcd, spare_fb->fbid);

    rt_mutex_take(g_lck_fblist, RT_WAITING_NO);
    spare_fb->tags = FB_TAG_READY;
    rt_sem_release(g_sem_ready);
    rt_mutex_release(g_lck_fblist);

    rt_err_t rt_thread_yield(void);
    rt_thread_yield();
  }
  return ret;
}

static void fbswap_thread(void* ctx) {
  fb_info_t* fb = &g_fb;
  int fbid;

  while (!g_app_quited) {
    rt_sem_take(g_sem_ready, RT_WAITING_FOREVER);

    rt_mutex_take(g_lck_fblist, RT_WAITING_NO);
    fb_taged_t* ready_fb = get_ready_fb();
    assert(ready_fb);

    rt_mutex_release(g_lck_fblist);

    fbid = ready_fb->fbid;

    mpp_fb_ioctl(fb->fb, AICFB_PAN_DISPLAY, &fbid);

    aic_fb_enable();

    mpp_fb_ioctl(fb->fb, AICFB_WAIT_FOR_VSYNC, 0);

    rt_mutex_take(g_lck_fblist, RT_WAITING_NO);
    fb_taged_t* last_busy_fb = get_busy_fb();
    if (last_busy_fb) {
      last_busy_fb->tags = FB_TAG_SPARE;
      rt_sem_release(g_sem_spare);
    }
    ready_fb->tags = FB_TAG_BUSY;
    rt_mutex_release(g_lck_fblist);
#ifdef TK_USE_AIC_FPS
  cal_frame_rate();
#endif
  }

  log_info("display_thread end\n");
}
#endif

static lcd_t* lcd_rtos_single_framebuffer_create(fb_info_t* fb) {
  lcd_t* lcd = NULL;
  int w = fb_width(fb);
  int h = fb_height(fb);
  int line_length = fb_line_length(fb);

  int bpp = fb_bpp(fb);
  uint8_t* online_fb = (uint8_t*)(fb->fbmem0);
  uint8_t* offline_fb = NULL;
  return_value_if_fail(offline_fb != NULL, NULL);

  offline_fb = (uint8_t*)malloc(line_length * h);

  if (bpp == 16) {
    if (fb_is_bgr565(fb)) {
      lcd = lcd_mem_bgr565_create_double_fb(w, h, online_fb, offline_fb);
    } else if (fb_is_rgb565(fb)) {
      lcd = lcd_mem_rgb565_create_double_fb(w, h, online_fb, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 32) {
    if (fb_is_bgra8888(fb)) {
      lcd = lcd_mem_bgra8888_create_double_fb(w, h, online_fb, offline_fb);
    } else if (fb_is_rgba8888(fb)) {
      lcd = lcd_mem_rgba8888_create_double_fb(w, h, online_fb, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 24) {
    if (fb_is_bgr888(fb)) {
      lcd = lcd_mem_bgr888_create_double_fb(w, h, online_fb, offline_fb);
    } else if (fb_is_rgb888(fb)) {
      lcd = lcd_mem_rgb888_create_double_fb(w, h, online_fb, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else {
    assert(!"not supported framebuffer format.");
  }

  if (lcd != NULL) {
    lcd_mem_rtos_flush_default = lcd->flush;
    lcd->flush = lcd_mem_rtos_flush;
    lcd_mem_set_line_length(lcd, line_length);
  }

  return lcd;
}

static lcd_t* lcd_mem_create_fb(fb_info_t* fb)
{
  int w = fb_width(fb);
  int h = fb_height(fb);
  int bpp = fb_bpp(fb);
  lcd_t* lcd = NULL;
#ifdef WITH_LCD_FLUSH_AND_SWAP
  uint8_t* offline_fb = (uint8_t*)(fb->fbmem_offline);
  return_value_if_fail(offline_fb != NULL, NULL);

  if (bpp == 16) {
    if (fb_is_bgr565(fb)) {
      lcd = lcd_mem_bgr565_create_single_fb(w, h, offline_fb);
    } else if (fb_is_rgb565(fb)) {
      lcd = lcd_mem_rgb565_create_single_fb(w, h, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 32) {
    if (fb_is_bgra8888(fb)) {
      lcd = lcd_mem_bgra8888_create_single_fb(w, h, offline_fb);
    } else if (fb_is_rgba8888(fb)) {
      lcd = lcd_mem_rgba8888_create_single_fb(w, h, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 24) {
    if (fb_is_bgr888(fb)) {
      lcd = lcd_mem_bgr888_create_single_fb(w, h, offline_fb);
    } else if (fb_is_rgb888(fb)) {
      lcd = lcd_mem_rgb888_create_single_fb(w, h, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else {
    assert(!"not supported framebuffer format.");
  }
/* flush / swap / swap_ex mode */
#else
  if (bpp == 16) {
    log_info("lcd mem format is bgr565\n");
    if (fb_is_bgr565(fb)) {
      lcd = lcd_mem_bgr565_create_double_fb(w, h, fb->fbmem0, fb->fbmem_offline);
    } else if (fb_is_rgb565(fb)) {
      lcd = lcd_mem_rgb565_create_double_fb(w, h, fb->fbmem0, fb->fbmem_offline);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 32) {
    log_info("lcd mem format is bgra8888\n");
    if (fb_is_bgra8888(fb)) {
      lcd = lcd_mem_bgra8888_create_double_fb(w, h, fb->fbmem0, fb->fbmem_offline);
    } else if (fb_is_rgba8888(fb)) {
      lcd = lcd_mem_rgba8888_create_double_fb(w, h, fb->fbmem0, fb->fbmem_offline);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 24) {
    log_info("lcd mem format is bgr888\n");
    if (fb_is_bgr888(fb)) {
      lcd = lcd_mem_bgr888_create_double_fb(w, h, fb->fbmem0, fb->fbmem_offline);
    } else if (fb_is_rgb888(fb)) {
      lcd = lcd_mem_rgb888_create_double_fb(w, h, fb->fbmem0, fb->fbmem_offline);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else {
    assert(!"not supported framebuffer format.");
  }
#endif
  return lcd;
}

static lcd_t* lcd_rtos_double_framebuffer_create(fb_info_t* fb) {
  lcd_t* lcd = NULL;
  int h = fb_height(fb);
  int line_length = fb_line_length(fb);
#ifdef WITH_AIC_G2D
  int ret = 0;

  tk_aic_g2d_open();

  ret = aic_cma_buf_open();
  if (ret < 0) {
    assert("aic_cma_buf_open err\n");
  }

  lcd_buffer[0].type = PHY_TYPE;
  lcd_buffer[0].buf_head = (uintptr_t)fb->fbmem0; /* in an rtos, the physical address is essentially the pointer address. */
  lcd_buffer[0].phy_addr = (uintptr_t)fb->fbmem0;
  lcd_buffer[0].buf = (void *)fb->fbmem0;
  lcd_buffer[0].size = line_length * h;

  aic_cma_buf_add_ge(&lcd_buffer[0]);
  if (fb_is_2fb(fb)) {
    lcd_buffer[1].type = PHY_TYPE;
    lcd_buffer[1].buf_head = (uintptr_t)fb->fbmem0 + (line_length * h);
    lcd_buffer[1].phy_addr = (uintptr_t)(fb->fbmem0 + (line_length * h));
    lcd_buffer[1].buf = (void *)((uintptr_t)(fb->fbmem0 + (line_length * h)));
    lcd_buffer[1].size = line_length * h;
    aic_cma_buf_add_ge(&lcd_buffer[1]);

#ifdef WITH_LCD_FLUSH_AND_SWAP
    ret = aic_cma_buf_malloc(&lcd_buffer[2], h * line_length, 0);
    if (ret < 0) {
      assert("aic_cma_buf_malloc err\n");
    }

    if (ret >= 0) {
      fb->fbmem_offline = lcd_buffer[2].buf;
    }

    aic_cma_buf_add_ge(&lcd_buffer[2]);
#endif
  }

  /* aic_cma_buf_debug(AIC_CMA_BUF_DEBUG_SIZE | AIC_CMA_BUF_DEBUG_CONTEXT); */
#endif

  lcd = lcd_mem_create_fb(fb);

#ifdef WITH_LCD_FLUSH
  if (lcd != NULL) {
    log_debug("lcd mode is flush\n");
    lcd_mem_rtos_flush_default = lcd->flush;
    lcd->flush = lcd_mem_rtos_flush;
  }
#endif

#ifdef WITH_LCD_SWAP
  log_debug("lcd mode is swap\n");
  /* Only using the double framebuffer assigned by the platform */
  if (lcd != NULL) {
    lcd->swap = lcd_mem_rtos_swap;
    lcd->support_dirty_rect = FALSE;
  } else {
    assert(!"lcd create err.\n");
  }
#endif

#ifdef WITH_LCD_SWAP_EX
  log_debug("lcd mode is swap ex\n");
  /* Only using the double framebuffer assigned by the platform */
  if (lcd != NULL) {
    lcd->swap = lcd_mem_rtos_swap_ex;
    lcd->support_dirty_rect = TRUE;
  } else {
    assert(!"lcd create err.\n");
  }
#endif

#ifdef WITH_LCD_FLUSH_AND_SWAP
  log_debug("lcd mode is flush and swap\n");
  init_fblist(fb_number(fb));
  uint8_t* offline_fb = (uint8_t*)(fb->fbmem_offline);
  return_value_if_fail(offline_fb != NULL, NULL);

  if (lcd != NULL) {
    lcd->swap = lcd_mem_rtos_write_buff;
    lcd->flush = lcd_mem_rtos_write_buff;
    lcd_mem_set_line_length(lcd, line_length);

    g_lck_fblist = rt_mutex_create("awtk-draw-mutex", 0);
    g_sem_spare = rt_sem_create("awtk-sem-spare", fb_number(fb), 0);
    g_sem_ready = rt_sem_create("awtk-sem-ready", 0, 0);

    g_bswap = rt_thread_create("awtk-draw", fbswap_thread, NULL, 1024 * 2,
                                    TK_UI_THREAD_PRIORITY, 20);
    rt_thread_startup(g_bswap);
  }
#endif

  return lcd;
}

lcd_t* platform_create_lcd(wh_t w, wh_t h) {
  lcd_t* lcd = NULL;
  fb_info_t* fb = &g_fb;

  if (tk_fb_disp_init(fb) == RET_OK) {
    if (fb_is_1fb(fb)) {
      return lcd_rtos_single_framebuffer_create(fb);
    } else {
      return lcd_rtos_double_framebuffer_create(fb);
    }
  } else {
    on_app_exit();
  }

  return lcd;
}
