#include <Arduino.h>
#include "device_lib_exports.h"
#include "lvgl.h"
#include "esp_lcd_touch_axs15231b.h"
#include "esp_log.h"
#include "PMU_AXP2101.h"

#ifndef GPIO_BCKL
#define GPIO_BCKL 6
#endif

extern void set_brightness(float level);

void Backlight_Init()
{
    // Nothing special to do here
}

void Set_Backlight(uint8_t Light)
{
    if (Light > 100)
        Light = 100;

    uint32_t duty = (255u * Light) / 100u;

    pinMode(GPIO_BCKL, OUTPUT);
    analogWrite(GPIO_BCKL, duty);
    
    Serial.printf("[BACKLIGHT] Set to %d%% (duty=%d)\n", Light, duty);
}

void I2C_Init(void)
{
    static bool init_done = false;
    if (init_done) {
        return;
    }

    // Assume Wire / I2C hardware were already initialized by core / board init.
    // Just ensure the clock is what we want.
    Wire.setClock(400000);  // 400 kHz

    Serial.println("[I2C] Using SDA=8, SCL=7 (clock=400kHz)");
    init_done = true;
}


void LCD_Init()
{
    // Legacy hook
}

extern "C" lv_display_t *lvgl_lcd_init_perf(void);
extern "C" lv_indev_t *indev;

touch_and_screen Lvgl_Init(void)
{
    touch_and_screen tas{};

    Serial.println("[LVGL_INIT] Starting initialization...");

    

    // ✅ Ensure LVGL core is initialized once
    static bool lv_initialized = false;
    if (!lv_initialized) {
        lv_init();
        lv_initialized = true;
        Serial.println("[LVGL_INIT] lv_init() called");
    } else {
        Serial.println("[LVGL_INIT] lv_init() already called, skipping");
    }

    // Make sure the shared I2C bus is up before LVGL/touch init.
    I2C_Init();

    // Set backlight to full before initializing display
    Serial.println("[LVGL_INIT] Setting backlight to 100%");
    Set_Backlight(100);
    delay(100);

    // Call our perf panel driver init
    Serial.println("[LVGL_INIT] Calling lvgl_lcd_init_perf()...");
    tas.screen = lvgl_lcd_init_perf();
    tas.touch  = indev;

    if (tas.screen == nullptr) {
        Serial.println("[LVGL_INIT] ERROR: Display creation failed!");
    } else {
        Serial.println("[LVGL_INIT] Display created successfully");
    }

    if (tas.touch == nullptr) {
        Serial.println("[LVGL_INIT] ERROR: Touch input device is NULL!");
    } else {
        Serial.println("[LVGL_INIT] Touch device created successfully");
    }

    // Force a screen refresh
    Serial.println("[LVGL_INIT] Forcing screen refresh...");
    lv_refr_now(tas.screen);
    
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
