// device_lib_exports.h - ST7796 + FT6336 Configuration
#pragma once

#include <lvgl.h>

// Display configuration
// #ifndef DISPLAY_WIDTH
//   #define DISPLAY_WIDTH  320
// #endif
// #ifndef DISPLAY_HEIGHT
//   #define DISPLAY_HEIGHT 480
// #endif

#define LCD_WIDTH  DISPLAY_WIDTH
#define LCD_HEIGHT DISPLAY_HEIGHT

// I2C pins
// #ifndef I2C_SDA
//   #define I2C_SDA 8
// #endif
// #ifndef I2C_SCL
//   #define I2C_SCL 7
// #endif

// Touch header
#include "files/esp_lcd_touch_ft6336.h"

// Power key and battery definitions
#define PWR_KEY_Input_PIN   -0
#define PWR_Control_PIN     -0  // Set to actual pin if you have a latch

// Map to generic names that PWR_Key.cpp expects
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

// Struct for LVGL initialization
struct touch_and_screen
{
    lv_indev_t *touch;
    lv_display_t *screen;
};

// Function exports
void Backlight_Init();
void Set_Backlight(uint8_t Light);
void I2C_Init(void);
void LCD_Init();
touch_and_screen Lvgl_Init(void);

void BAT_Init(void);
float BAT_Get_Volts(void);

// Low-level power key / latch HAL (implemented in files/PWR_Key.cpp)
void power_key_setup(void);
bool power_key_pressed(void);

void power_latch_on(void);
void power_latch_off(void);

void power_enable_wakeup_lightsleep(void);
void power_disable_wakeup_lightsleep(void);
void power_enable_wakeup_deepsleep(void);