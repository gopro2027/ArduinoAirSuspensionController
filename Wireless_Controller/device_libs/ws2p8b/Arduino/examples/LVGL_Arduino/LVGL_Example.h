#pragma once

#include "LVGL_Driver.h"
#include "Display_ST7701.h"  
#include "Gyro_QMI8658.h"
#include "RTC_PCF85063.h"
#include "SD_Card.h"
#include "BAT_Driver.h"
#include "Wireless.h"

#define EXAMPLE1_LVGL_TICK_PERIOD_MS  1000


void Backlight_adjustment_event_cb(lv_event_t * e);

void Lvgl_Example1(void);
void LVGL_Backlight_adjustment(uint8_t Backlight);