#pragma once

#include <lvgl.h>

#define LCD_WIDTH  DISPLAY_WIDTH
#define LCD_HEIGHT DISPLAY_HEIGHT

// requirements for screen
#include "files/esp_lcd_touch_axs15231b.h"

// power key and battery definitions
// Note: update these for the 3.5b as needed
#define PWR_KEY_Input_PIN   -0
#define PWR_Control_PIN     -0

// Map to the generic names that the ws3p5b PWR_Key.cpp expects
#ifndef PWR_KEY_PIN
#define PWR_KEY_PIN PWR_KEY_Input_PIN
#endif

#ifndef PWR_LATCH_PIN
#define PWR_LATCH_PIN PWR_Control_PIN
#endif

#ifndef PWR_LATCH_ACTIVE_LEVEL
#define PWR_LATCH_ACTIVE_LEVEL HIGH
#endif

#ifndef PWR_KEY_ACTIVE_LOW
#define PWR_KEY_ACTIVE_LOW 1
#endif

// struct used for Lvgl_Init
struct touch_and_screen
{
    lv_indev_t *touch;
    lv_display_t *screen;
};

// NOTE: need these functions implemented somewhere in the files folder. See 2p8 and 2p8b for examples
void Backlight_Init();
void Set_Backlight(uint8_t Light);
void I2C_Init(void);  // NOTE: can be set to {} if it just doesn't exist on the 3.5b. Otherwise, please define it appropriately
void LCD_Init();
touch_and_screen Lvgl_Init(void);

void BAT_Init(void);
float BAT_Get_Volts(void);

// ───────── Low-level power key / latch HAL (implemented in files/PWR_Key.cpp) ─────────

void power_key_setup(void);
bool power_key_pressed(void);

void power_latch_on(void);
void power_latch_off(void);

void power_enable_wakeup_lightsleep(void);
void power_disable_wakeup_lightsleep(void);
void power_enable_wakeup_deepsleep(void);
