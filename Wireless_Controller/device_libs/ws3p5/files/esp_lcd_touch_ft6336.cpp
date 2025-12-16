// esp_lcd_touch_ft6336.cpp
// Touch implementation for FT6336 using TouchDrvFT6X36 library

#include "esp_lcd_touch_ft6336.h"
#include <TouchDrvFT6X36.hpp>
#include <Wire.h>

static TouchDrvFT6X36 touch_driver;
static bool touch_initialized = false;

void bsp_touch_init(TwoWire *touch_i2c, int tp_rst, uint16_t rotation, uint16_t width, uint16_t height)
{
    if (touch_initialized) {
        return;
    }

    if (!touch_driver.begin(*touch_i2c, FT6336_SLAVE_ADDRESS)) {
        Serial.println("[TOUCH] FT6336 initialization failed!");
        return;
    }

    touch_initialized = true;
    Serial.println("[TOUCH] FT6336 initialized via bsp_touch_init");
}

void bsp_touch_read(void)
{
    // The TouchDrvFT6X36 library handles reading internally
    // Nothing to do here
}

bool bsp_touch_get_coordinates(touch_data_t *touch_data)
{
    if (!touch_initialized || !touch_data) {
        return false;
    }

    int16_t x[MAX_TOUCH_MAX_POINTS];
    int16_t y[MAX_TOUCH_MAX_POINTS];
    
    uint8_t touched = touch_driver.getPoint(x, y, MAX_TOUCH_MAX_POINTS);
    
    if (touched > 0) {
        touch_data->touch_num = touched;
        for (uint8_t i = 0; i < touched && i < MAX_TOUCH_MAX_POINTS; i++) {
            touch_data->coords[i].x = x[i];
            touch_data->coords[i].y = y[i];
        }
        return true;
    }
    
    return false;
}

void touch_print_status(void)
{
    int16_t x[1], y[1];
    uint8_t touched = touch_driver.getPoint(x, y, 1);
    
    Serial.printf("[TOUCH] Status: %s at (%d, %d)\n",
                  touched ? "PRESSED" : "RELEASED",
                  touched ? x[0] : 0,
                  touched ? y[0] : 0);
}