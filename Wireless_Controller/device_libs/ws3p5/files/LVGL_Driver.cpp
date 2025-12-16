// LVGL_Driver.cpp - Cleaned for ST7796 + FT6336
#include <Arduino.h>
#include <Wire.h>

#include "device_lib_exports.h"
#include "lvgl.h"
#include "esp_lcd_touch_ft6336.h"
#include "PMU_AXP2101.h"

#ifndef GPIO_BCKL
  #define GPIO_BCKL 6
#endif

extern void set_brightness(float level);

// indev is DEFINED elsewhere (board_driver_util.cpp in your project)
extern "C" lv_indev_t *indev;

// Panel init entrypoint
extern "C" lv_display_t *lvgl_lcd_init_perf(void);

void Backlight_Init()
{
    // no-op
}

void Set_Backlight(uint8_t Light)
{
    if (Light > 100) Light = 100;

    pinMode(GPIO_BCKL, OUTPUT);
    digitalWrite(GPIO_BCKL, (Light > 0) ? HIGH : LOW);

    Serial.printf("[BACKLIGHT] GPIO Set to %u%%\n", (unsigned)Light);
}

void I2C_Init(void)
{
    static bool init_done = false;
    if (init_done) {
        Serial.println("[I2C] Already initialized, skipping");
        return;
    }

    // assume bus already started elsewhere; just set clock
    Wire.setClock(400000);
    Serial.println("[I2C] Clock set to 400kHz (bus assumed already started)");
    init_done = true;
}

void LCD_Init()
{
    Serial.println("[LCD_Init] Called (init handled by lvgl_lcd_init_perf)");
}

touch_and_screen Lvgl_Init(void)
{
    touch_and_screen tas{};

    Serial.println("[LVGL_INIT] Starting ST7796 + FT6336 initialization...");

    static bool lv_initialized = false;
    if (!lv_initialized) {
        lv_init();
        lv_initialized = true;
        Serial.println("[LVGL_INIT] lv_init() called");
    }

    I2C_Init();

    Serial.println("[LVGL_INIT] Calling lvgl_lcd_init_perf()...");
    tas.screen = lvgl_lcd_init_perf();
    tas.touch  = indev;

    if (!tas.screen) Serial.println("[LVGL_INIT] ERROR: Display creation failed!");
    else            Serial.println("[LVGL_INIT] Display created successfully");

    if (!tas.touch) Serial.println("[LVGL_INIT] ERROR: Touch input device is NULL!");
    else            Serial.println("[LVGL_INIT] Touch device created successfully");

    if (tas.screen) {
        lv_obj_invalidate(lv_screen_active());
    }

    Serial.println("[LVGL_INIT] Initialization complete");
    return tas;
}

void BAT_Init(void)
{
    PMU_init();
}

float BAT_Get_Volts(void)
{
    uint16_t mv = PMU_getBatteryVoltage_mV();
    return mv / 1000.0f;
}
