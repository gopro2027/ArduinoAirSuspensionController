// esp_lcd_touch_ft6336.h
#pragma once

#include <Arduino.h>
#include <Wire.h>

#define MAX_TOUCH_MAX_POINTS    2
#define FT6336_SLAVE_ADDRESS    0x38

typedef struct {
    uint16_t x;
    uint16_t y;
} coords_t;

typedef struct {
    coords_t coords[MAX_TOUCH_MAX_POINTS];
    uint8_t touch_num;
} touch_data_t;

// Touch API functions
void bsp_touch_init(TwoWire *touch_i2c, int tp_rst, uint16_t rotation, uint16_t width, uint16_t height);
void bsp_touch_read(void);
bool bsp_touch_get_coordinates(touch_data_t *touch_data);

// Diagnostic functions
void touch_print_status(void);