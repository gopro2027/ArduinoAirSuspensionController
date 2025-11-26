#ifndef board_driver_util_h
#define board_driver_util_h

#include <Arduino.h>
#include <lvgl.h>
#include <display/lv_display_private.h>
#include <misc/lv_timer_private.h>
#include <indev/lv_indev_private.h>

#include "device_lib_exports.h"

// Structure to store the data from the three point calibration data
typedef struct
{
    bool valid;
    float alphaX;
    float betaX;
    float deltaX;
    float alphaY;
    float betaY;
    float deltaY;
} touch_calibration_data_t;

// Touch calibration
extern touch_calibration_data_t touch_calibration_data;
touch_calibration_data_t smartdisplay_compute_touch_calibration(const lv_point_t screen[3], const lv_point_t touch[3]);

void set_brightness(float level);
void board_drivers_init();

#endif