#pragma once

#include "lvgl.h"
#include "demos/lv_demos.h"

#include "LVGL_Driver.h"
#include "ST7701S.h"  
#include "TCA9554PWR.h"
#include "PCF85063.h"
#include "QMI8658.h"
#include "SD_MMC.h"
#include "Wireless.h"
#include "Buzzer.h"
#include "BAT_Driver.h"

#define EXAMPLE1_LVGL_TICK_PERIOD_MS  1000


void Backlight_adjustment_event_cb(lv_event_t * e);

void Lvgl_Example1(void);
void LVGL_Backlight_adjustment(uint8_t Backlight);