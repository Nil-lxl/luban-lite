/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>

#define PWM0                                 0
#define PWM1                                 1
#define PWM2                                 2
#define PWM3                                 3
#define EPWM0                                0
#define EPWM1                                1
#define EPWM2                                2
#define EPWM3                                3
#define EPWM4                                4
#define EPWM5                                5
#define EPWM6                                6
#define EPWM7                                7
#define EPWM8                                8
#define EPWM9                                9
#define EPWM10                               10
#define EPWM11                               11

#define REAR_INDICATOR_LIGHT                 EPWM2
#define SYSTEM_IDNICATOR_LIGHT               EPWM5
#define FRONT_INDICATOR_LIGHT                EPWM8
#define BATTERY_LVL_DISPLAY_LIGHT1           PWM0
#define BATTERY_LVL_DISPLAY_LIGHT2           PWM1
#define BATTERY_LVL_DISPLAY_LIGHT3           PWM2
#define BATTERY_LVL_DISPLAY_LIGHT4           PWM3

#define LEFT_UPPER_ARM_CTL                   EPWM0
#define LEFT_LOWER_ARM_CTL                   EPWM0
#define RIGHT_UPPER_ARM_CTL                  EPWM1
#define RIGHT_LOWER_ARM_CTL                  EPWM1

#define PAN_TILT_MOTOR_CTL_INERFACE_GROUP_A1 EPWM3
#define PAN_TILT_MOTOR_CTL_INERFACE_GROUP_A2 EPWM3
#define PAN_TILT_MOTOR_CTL_INERFACE_GROUP_A3 EPWM4

#define PAN_TILT_MOTOR_CTL_INERFACE_GROUP_B1 EPWM6
#define PAN_TILT_MOTOR_CTL_INERFACE_GROUP_B2 EPWM6
#define PAN_TILT_MOTOR_CTL_INERFACE_GROUP_B3 EPWM7

#define PAN_TILT_MOTOR_CTL_INERFACE_GROUP_C1 EPWM9
#define PAN_TILT_MOTOR_CTL_INERFACE_GROUP_C2 EPWM9
#define PAN_TILT_MOTOR_CTL_INERFACE_GROUP_C3 EPWM10

#define HEATING_PAN_TILT_GYROSCOPE           EPWM11

int test_multipwm(int argc, char **argv)
{
    struct rt_device_pwm *pwm_dev = RT_NULL;
    struct rt_device_pwm *epwm_dev = RT_NULL;

    pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm");
    epwm_dev = (struct rt_device_pwm *)rt_device_find("epwm");

    if (pwm_dev == NULL) {
        rt_kprintf("can't find pwm device!\n");
        return -RT_ERROR;
    }

    if (epwm_dev == NULL) {
        rt_kprintf("can't find Epwm device!\n");
        return -RT_ERROR;
    }

    //rear indicator light
    rt_pwm_set(epwm_dev, REAR_INDICATOR_LIGHT, 10000, 5000);//prd:10000ns, duty_ns:5000ns; freq=1/prd = 100KHz duty=50%
    rt_pwm_enable(epwm_dev, REAR_INDICATOR_LIGHT);

    //system indicator light
    rt_pwm_set(epwm_dev, SYSTEM_IDNICATOR_LIGHT, 1000, 300);//prd:1000ns, duty_ns:300ns; freq=1/prd = 1000KHz duty=30%
    rt_pwm_enable(epwm_dev, SYSTEM_IDNICATOR_LIGHT);

    //front indicator light
    rt_pwm_set(epwm_dev, FRONT_INDICATOR_LIGHT, 2000, 500);
    rt_pwm_enable(epwm_dev, FRONT_INDICATOR_LIGHT);

    //battery level display light
    rt_pwm_set(pwm_dev, BATTERY_LVL_DISPLAY_LIGHT1, 3000, 500);
    rt_pwm_enable(pwm_dev, BATTERY_LVL_DISPLAY_LIGHT1);
    rt_pwm_set(pwm_dev, BATTERY_LVL_DISPLAY_LIGHT2, 4000, 500);
    rt_pwm_enable(pwm_dev, BATTERY_LVL_DISPLAY_LIGHT2);
    rt_pwm_set(pwm_dev, BATTERY_LVL_DISPLAY_LIGHT3, 5000, 500);
    rt_pwm_enable(pwm_dev, BATTERY_LVL_DISPLAY_LIGHT3);
    rt_pwm_set(pwm_dev, BATTERY_LVL_DISPLAY_LIGHT4, 6000, 500);
    rt_pwm_enable(pwm_dev, BATTERY_LVL_DISPLAY_LIGHT4);

    /* According to the hardware design, EPWM0/3/6/9 is the main synchronizer,
     * and using synchronization mode to enable it needs to be placed later.
     */

    //arm control
    //rt_pwm_set_output used to set different outputs of an EPWM
    rt_pwm_set_output(epwm_dev, LEFT_UPPER_ARM_CTL, 100000, 50000, 0);//EPWM0:output0 10KHz 50%
    rt_pwm_set_output(epwm_dev, LEFT_UPPER_ARM_CTL, 100000, 20000, 1);//EPWM1:output1 10KHz 20%
    rt_pwm_set_output(epwm_dev, RIGHT_UPPER_ARM_CTL, 100000, 80000, 0);//EPWM0:output0 10KHz 80%
    rt_pwm_set_output(epwm_dev, RIGHT_UPPER_ARM_CTL, 100000, 30000, 1);//EPWM1:output1 10KHz 30%
    rt_pwm_enable(epwm_dev, RIGHT_UPPER_ARM_CTL);
    rt_pwm_enable(epwm_dev, LEFT_UPPER_ARM_CTL);//EPWM0

    //pan tilt motor control 1
    //rt_pwm_set used to set same outputs of an EPWM
    rt_pwm_set(epwm_dev, PAN_TILT_MOTOR_CTL_INERFACE_GROUP_A1, 100000, 20000);//EPWM3:output0/1 10KHz 20%
    rt_pwm_set(epwm_dev, PAN_TILT_MOTOR_CTL_INERFACE_GROUP_A3, 100000, 20000);//EPWM4:output0/1 10KHz 20%
    rt_pwm_enable(epwm_dev, PAN_TILT_MOTOR_CTL_INERFACE_GROUP_A3);
    rt_pwm_enable(epwm_dev, PAN_TILT_MOTOR_CTL_INERFACE_GROUP_A1);//EPWM3

    //pan tilt motor control 2
    rt_pwm_set(epwm_dev, PAN_TILT_MOTOR_CTL_INERFACE_GROUP_B1, 100000, 50000);
    rt_pwm_set(epwm_dev, PAN_TILT_MOTOR_CTL_INERFACE_GROUP_B3, 100000, 50000);
    rt_pwm_enable(epwm_dev, PAN_TILT_MOTOR_CTL_INERFACE_GROUP_B3);
    rt_pwm_enable(epwm_dev, PAN_TILT_MOTOR_CTL_INERFACE_GROUP_B1);//EPWM6

    //pan tilt motor control 3
    rt_pwm_set(epwm_dev, PAN_TILT_MOTOR_CTL_INERFACE_GROUP_C1, 100000, 80000);
    rt_pwm_set(epwm_dev, PAN_TILT_MOTOR_CTL_INERFACE_GROUP_C3, 100000, 80000);
    rt_pwm_enable(epwm_dev, PAN_TILT_MOTOR_CTL_INERFACE_GROUP_C3);
    rt_pwm_enable(epwm_dev, PAN_TILT_MOTOR_CTL_INERFACE_GROUP_C1);//EPWM9

    //heating of pan tilt gyroscope
    rt_pwm_set(epwm_dev, HEATING_PAN_TILT_GYROSCOPE, 1000, 500);
    rt_pwm_enable(epwm_dev, HEATING_PAN_TILT_GYROSCOPE);

    return RT_EOK;
}
MSH_CMD_EXPORT_ALIAS(test_multipwm, test_multipwm, Test the epwm and pwm);
