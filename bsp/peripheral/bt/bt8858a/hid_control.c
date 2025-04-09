#include <rtthread.h>
#include "bt_api.h"
#include "hid_control.h"

#define BT_TOUCH_EVENT_MOVE     0
#define BT_TOUCH_EVENT_DOWN     1
#define BT_TOUCH_EVENT_UP       2

static coordinate_t gt_device;
static coordinate_t gt_host;

static coordinate_t gt_currunt_host_pos;

static float Px = 0.0f;
static float Py = 0.0f;

static void hid_connnect_cb(int status)
{
    coordinate_t device = {600, 1024};
    coordinate_t host = {1080, 2400};

    if (status == BT_HID_STATE_CONNECTED) {
        bt_hid_set_resolution(&device, &host);
        printf("HID connected! set device resulo = %d * %d, host resulo = %d * %d\n",
                 device.x, device.y, host.x, host.y);
    } else {
        printf("HID disconnected\n");
    }
}

int bt_hid_device_init()
{
    printf("BT hid start\n");
    bt_init();
    msleep(10);
    bt_hid_connect();

    bt_hid_request_connect_cb(hid_connnect_cb);

    return 0;
}

int bt_hid_device_deinit()
{
    bt_hid_disconnect();
    bt_exit();
    return 0;
}

/* must be set after hid connected */
int bt_hid_set_resolution(coordinate_t *device_resolu, coordinate_t *host_resolu)
{
    gt_device.x = device_resolu->x;
    gt_device.y = device_resolu->y;

    gt_host.x = host_resolu->x;
    gt_host.y = host_resolu->y;

    Px = (float)gt_host.x / (float)gt_device.x;
    Py = (float)gt_host.y / (float)gt_device.y;

    printf("Px = %f; Py = %f\n", Px, Py);

    bt_hid_set_cursor_pos(host_resolu->x, host_resolu->y);
    msleep(20);
    bt_hid_reset_cursor_pos();

    gt_currunt_host_pos = gt_host;
}

int bt_hid_set_touch_event(int status, u16 x_device, u16 y_device)
{
    s16 x_diff;
    s16 y_diff;
    s16 x;
    s16 y;

    if (!bt_hid_is_connected()) {
        printf("HID is not connect\n");
        return -1;
    }


    x = ((float)x_device * Px);
    y = ((float)y_device * Py);

    // printf("touch : [%d] device: (%d, %d), host: (%d, %d)\n", status, x_device, y_device, x, y);

    if (status == TP_STATUS_UP) {
        x_diff = x - gt_currunt_host_pos.x;
        y_diff = y - gt_currunt_host_pos.y;

        gt_currunt_host_pos = gt_host;

        bt_hid_set_cursor_xy(BT_TOUCH_EVENT_UP, x_diff, y_diff);
        msleep(20);

        bt_hid_reset_cursor_pos();
    } else {
        x_diff = x - gt_currunt_host_pos.x;
        y_diff = y - gt_currunt_host_pos.y;

        gt_currunt_host_pos.y = y;
        gt_currunt_host_pos.x = x;

        bt_hid_set_cursor_xy(BT_TOUCH_EVENT_DOWN, x_diff, y_diff);
    }

}

INIT_DEVICE_EXPORT(bt_hid_device_init);

void bt_set(int argc, char *argv[])
{
    coordinate_t device = {600, 1024};
    coordinate_t host = {1080, 2400};

    bt_hid_set_resolution(&device, &host);
    printf("HID connected! set device resulo = %d * %d, host resulo = %d * %d\n",
                device.x, device.y, host.x, host.y);
}
MSH_CMD_EXPORT_ALIAS(bt_set, bt_set, debug bt command);
