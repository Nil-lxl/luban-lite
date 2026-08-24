#pragma once

typedef enum KEY_FLAG {
    KEY_NONE=0,
    KEY_UP = 1,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
}key_flag_t;

void keyadc_device_enable(int channel);

void keyadc_device_disable(int channel);

key_flag_t keyadc_get_flag(int channel, int scale);