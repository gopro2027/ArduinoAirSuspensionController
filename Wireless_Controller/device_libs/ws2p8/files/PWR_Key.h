#pragma once
#include "Arduino.h"
#include "Display_ST7789.h"

#define PWR_KEY_ACTIVE_LOW 1    // button pulls low when pressed
#define PWR_LATCH_ACTIVE_HIGH 1 // drive HIGH to hold power

#define PWR_KEY_Input_PIN   6
#define PWR_Control_PIN     7

#define Measurement_offset 0.990476     

void power_key_setup(void);
bool power_key_pressed(void);
void power_latch_on(void);
void power_latch_off(void);
void power_enable_wakeup_lightsleep();
void power_disable_wakeup_lightsleep();
void power_enable_wakeup_deepsleep();
// no need for power_disable_wakeup_deepsleep, deep sleep reboots on wake