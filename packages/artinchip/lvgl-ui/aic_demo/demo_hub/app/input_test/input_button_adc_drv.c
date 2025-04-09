#include "input_button_adc_drv.h"

#if defined(AIC_USING_GPAI) && defined(KERNEL_RTTHREAD)
#include "string.h"
#include "rtdevice.h"

static rt_adc_device_t gpai_dev;
static const char *gpai_name = "gpai";
static const int adc_chan = 7;

int button_adc_open(void)
{
    gpai_dev = (rt_adc_device_t)rt_device_find(gpai_name);
    if (!gpai_dev) {
        LV_LOG_ERROR("Failed to open %s device\n", gpai_name);
        return -1;
    }

    rt_adc_enable(gpai_dev, adc_chan);
    return 0;
}

void button_adc_close(void)
{
    if (gpai_dev)
        rt_adc_disable(gpai_dev, adc_chan);
}

static int button_debounce_deal(int button_id)
{
    const uint32_t debounce_delay = 40; /* ms */
    static uint32_t debounce_timer;
    static int last_button_id = -1;

    if (last_button_id != button_id) {
        debounce_timer = lv_tick_get();
        last_button_id = button_id;
    }

    if ((lv_tick_get() - debounce_timer) > debounce_delay) {
        return button_id;
    }

    return -1;
}

static int button_adc_read_id()
{
    const int button_adc_scale = 100;
    static int button_adc_arr[] = {413, 823, 1365, 1820, 2258, 2705, 3291, 3979};
    static int button_id[] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7}; /* ID must start from 0 */

    int adc_val = 0;

    adc_val = rt_adc_read(gpai_dev, adc_chan);

    for (int i = 0; i < sizeof(button_adc_arr) / sizeof(button_adc_arr[0]); i++)
        if ((button_adc_arr[i] - button_adc_scale) <= adc_val && adc_val <= (button_adc_arr[i] + button_adc_scale)) {
            LV_LOG_INFO("[button%d] ch%d: %d\n", button_id[i], adc_chan,
                           adc_val);
            return button_debounce_deal(button_id[i]);
    }

    return -1;
}

static void update_press_time(int btn_id, uint32_t *press_time)
{
    static int last_btn_id = -1;
    static uint32_t start_time = 0;

    if (last_btn_id != btn_id || btn_id < 0) {
        start_time = lv_tick_get();
    }

    *press_time = lv_tick_get() - start_time;

    last_btn_id = btn_id;
}

void button_adc_read(int *btn_id, uint32_t *btn_state)
{
    static int last_btn_id = -1;
    static uint32_t last_state = 0;
    static uint32_t press_time = 0;
    static uint32_t last_press_time = 0;
    static uint32_t state;

    int btn_pr = button_adc_read_id();
    if (btn_pr >= 0) {
        state = LV_EVENT_PRESSED;
    } else {
        state = LV_EVENT_RELEASED;
    }

    update_press_time(btn_pr, &press_time);

    if (last_state == LV_EVENT_PRESSED &&
        state == LV_EVENT_RELEASED &&
        last_press_time < 300) {
        state = LV_EVENT_SHORT_CLICKED;
        btn_pr = last_btn_id;
    }

    if (state == LV_EVENT_PRESSED && press_time > 1000) {
        state = LV_EVENT_LONG_PRESSED;
    }

    if (state == last_state && last_state == LV_EVENT_RELEASED) {
        state = 0;
    }

    if (last_state == 0 && state ==  LV_EVENT_RELEASED)
        state = 0;

    *btn_id = last_btn_id;
    *btn_state = state;
    last_state = state;
    last_press_time = press_time;
    last_btn_id = btn_pr;

    // rt_kprintf("id = %d, state = 0x%x, press_time = %d\n", *btn_id, *btn_state, press_time);
}
#endif
