/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch/esp_lcd_touch.h"

#include "TCA9554PWR.h"
#include "I2C_Driver.h"
#include "ST7701S.h"



/**
 * @brief Create a new GT911 touch driver
 *
 * @note The I2C communication should be initialized before use this function.
 *
 * @param io LCD/Touch panel IO handle
 * @param config: Touch configuration
 * @param out_touch: Touch instance handle
 * @return
 *      - ESP_OK                    on success
 *      - ESP_ERR_NO_MEM            if there is no memory for allocating main structure
 */
esp_err_t esp_lcd_touch_new_i2c_gt911(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config, esp_lcd_touch_handle_t *out_touch);

/**
 * @brief I2C address of the GT911 controller
 *
 */
#define ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS (0x5D)

/**
 * @brief Touch IO configuration structure
 *
 */
#define ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG()           \
    {                                       \
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS, \
        .control_phase_bytes = 1,           \
        .dc_bit_offset = 0,                 \
        .lcd_cmd_bits = 16,                 \
        .flags =                            \
        {                                   \
            .disable_control_phase = 1,     \
        }                                   \
    }

    

#define I2C_Touch_INT_IO            16         /*!< GPIO number used for I2C master data  */
#define I2C_Touch_RST_IO            -1         /*!< GPIO number used for I2C master clock */

extern esp_lcd_touch_handle_t tp;

void Touch_Init(void);
