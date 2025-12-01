#pragma once

#include <lvgl.h>

#define LCD_WIDTH DISPLAY_WIDTH
#define LCD_HEIGHT DISPLAY_HEIGHT

// requirements for screen
// #include "files/___.h"
#include "files/esp_lcd_touch_axs15231b.h"

// power key and battery definitions
// #include "files/___.h"
// Note: need to update these definitions for the 3.5b
#define BAT_ADC_PIN   4
#define Measurement_offset 0.992857
#define PWR_KEY_Input_PIN   6
#define PWR_Control_PIN     7

// struct used for Lvgl_Init
struct touch_and_screen
{
    lv_indev_t *touch;
    lv_display_t *screen;
};

// NOTE: need these functions implemented somewhere in the files folder. See 2p8 and 2p8b for examples
void Backlight_Init();
void Set_Backlight(uint8_t Light);
void I2C_Init(void); // NOTE: can be set to {} if it just doesn't exist on the 3.5b. Otherwise, please define it appropriately
void LCD_Init();
touch_and_screen Lvgl_Init(void);