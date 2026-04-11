#include "modbus_rtu_test.h"
#include "modbus.h"
#include "stdio.h"
#include "string.h"
#include "aic_hal_gpio.h"

#define RTS_PIN "PA.3"

#define GET_PIN(name) hal_gpio_name2pin(name)
#define RS485_RE GET_PIN(RTS_PIN)

int s_count = 0;
int f_count = 0;

static void test_thread(void *param)
{
    uint16_t tab_reg[64] = {0};
    modbus_t *ctx = RT_NULL;
    ctx = modbus_new_rtu("/dev/uart2", 115200, 'N', 8, 1);
    modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS485);
    modbus_rtu_set_rts(ctx, RS485_RE, MODBUS_RTU_RTS_UP);
    modbus_set_slave(ctx, 3);
    modbus_connect(ctx);
    modbus_set_response_timeout(ctx, 0, 1000000);
    modbus_set_debug(ctx, 1);
    int num = 0;
    while (1)
    {
        memset(tab_reg, 0, 64 * 2);
        int regs = modbus_read_registers(ctx, 0, 20, tab_reg);
        if (regs > 0) {
            printf("successful:%d\n", ++s_count);
        } else {
            printf("fail:%d\n", ++f_count);
        }
        printf("-------------------------------------------\n");
        printf("[%4d][read num = %d]", num, regs);
        num++;
        int i;
        for (i = 0; i < 20; i++)
        {
            printf("<%#x>", tab_reg[i]);
        }
        printf("\n");
        printf("-------------------------------------------\n");
        rt_thread_mdelay(10);
    }
    //7-关闭modbus端口
    modbus_close(ctx);

    //8-释放modbus资源
    modbus_free(ctx);
}

static int rtu_test_init(void)
{
    rt_pin_mode(RS485_RE, PIN_MODE_OUTPUT);
    rt_thread_t tid;
    tid = rt_thread_create("test",
                           test_thread, RT_NULL,
                           2048,
                           12, 10);
    if (tid != RT_NULL)
        rt_thread_startup(tid);
    return RT_EOK;
}
INIT_APP_EXPORT(rtu_test_init);
