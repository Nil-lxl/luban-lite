
#ifndef __HID_CONTROL_H__
#define __HID_CONTROL_H__

#include "bt_api.h"

typedef enum {
    TP_STATUS_NONE = 0,
    TP_STATUS_UP,
    TP_STATUS_DOWN,
    TP_STATUS_MOVE,
}TP_STATUS_T;

typedef struct {
    u16 x;
    u16 y;
}coordinate_t;

// typedef struct {
//     coordinate_t pointer;
//     TP_STATUS_T status;
// }action_t;

int bt_hid_device_init();
int bt_hid_device_deinit();
int bt_hid_set_resolution(coordinate_t *device_resolu, coordinate_t *host_resolu);
int bt_hid_set_touch_event(int status, u16 x_device, u16 y_device);
// int bt_hid_set_touch_event(action_t *action);

#endif
