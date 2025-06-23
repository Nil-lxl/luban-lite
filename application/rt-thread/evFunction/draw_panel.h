#pragma once

#include <rtthread.h>
#include <rtdevice.h>
#include "rtdbg.h"
#include "aic_core.h"
#include "mpp_fb.h"

struct frame_buffer_info
{
    int frame_buffer_format;
    int frame_buffer_width;
    int frame_buffer_height;
    unsigned char *frame_buffer;
};

struct line_dsc {
    int x1;
    int y1;
    int x2;
    int y2;
    int color;
    int width;
};

void open_panel(void);
void draw_start(void);