#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "lvgl.h"
#include "lv_demos.h"
#include "aic_ui.h"
#include "lv_port_disp.h"
#include "mpp_fb.h"
#include "app_ui.h"
#include "../thread/test_thread.h"


/**
 * Create a button with a label and react on click event.
 */
void app_ui_init() {
    // lv_demo_widgets();
    lv_demo_benchmark();
}
void ui_init(void) {
    app_ui_init();
}
