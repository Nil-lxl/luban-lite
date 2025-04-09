#include "lv_port_disp.h"
#include "mpp_fb.h"

int check_lvgl_de_config(void)
{
#if defined(AICFB_ARGB8888)
    #if LV_COLOR_DEPTH == 32
    return 0;
    #endif
#endif

#if defined(AICFB_RGB565)
    #if LV_COLOR_DEPTH == 16
    return 0;
    #endif
#endif

    return -1;
}

void fb_dev_layer_set(void)
{
#if defined(AICFB_RGB565)
    struct mpp_fb *fb = mpp_fb_open();
    struct aicfb_alpha_config alpha = {0};
    alpha.layer_id = 1;
    alpha.enable = 1;
    alpha.mode = 1;
    alpha.value = 255 / 2;
    mpp_fb_ioctl(fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    mpp_fb_close(fb);
#endif
}

void fb_dev_layer_reset(void)
{
#if defined(AICFB_RGB565)
    struct mpp_fb *fb = mpp_fb_open();

    struct aicfb_alpha_config alpha = {0};
    alpha.layer_id = 1;
    alpha.enable = 1;

    alpha.mode = 0;
    alpha.value = 255;

    mpp_fb_ioctl(fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    mpp_fb_close(fb);
#endif
}

void fb_dev_layer_only_video(void)
{
#if defined(AICFB_RGB565)
    struct mpp_fb *fb = mpp_fb_open();
    struct aicfb_alpha_config alpha = {0};
    alpha.layer_id = 1;
    alpha.enable = 1;
    alpha.mode = 1;
    alpha.value = 0;
    mpp_fb_ioctl(fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    mpp_fb_close(fb);
#endif
}

