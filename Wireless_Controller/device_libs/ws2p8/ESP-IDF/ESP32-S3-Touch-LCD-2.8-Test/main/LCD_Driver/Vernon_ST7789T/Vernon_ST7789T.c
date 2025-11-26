/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <sys/cdefs.h>
#include "sdkconfig.h"
#if CONFIG_LCD_ENABLE_DEBUG_LOG
// The local log level must be defined before including esp_log.h
// Set the maximum log level for this source file
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"

#include "Vernon_ST7789T/Vernon_ST7789T.h"

static const char *TAG = "lcd_panel.st7789t";

static esp_err_t panel_st7789t_del(esp_lcd_panel_t *panel);
static esp_err_t panel_st7789t_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_st7789t_init(esp_lcd_panel_t *panel);
static esp_err_t panel_st7789t_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data);
static esp_err_t panel_st7789t_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_st7789t_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_st7789t_swap_xy(esp_lcd_panel_t *panel, bool swap_axes);
static esp_err_t panel_st7789t_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap);
static esp_err_t panel_st7789t_disp_on_off(esp_lcd_panel_t *panel, bool off);

typedef struct {
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    bool reset_level;
    int x_gap;
    int y_gap;
    uint8_t fb_bits_per_pixel;
    uint8_t madctl_val; // save current value of LCD_CMD_MADCTL register
    uint8_t colmod_cal; // save surrent value of LCD_CMD_COLMOD register
} st7789t_panel_t;

esp_err_t esp_lcd_new_panel_st7789t(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_st7789t_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
#if CONFIG_LCD_ENABLE_DEBUG_LOG
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
#endif
    esp_err_t ret = ESP_OK;
    st7789t_panel_t *st7789t = NULL;
    ESP_GOTO_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    st7789t = calloc(1, sizeof(st7789t_panel_t));
    ESP_GOTO_ON_FALSE(st7789t, ESP_ERR_NO_MEM, err, TAG, "no mem for st7789t panel");

    if (panel_dev_config->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num,
        };
        ESP_GOTO_ON_ERROR(gpio_config(&io_conf), err, TAG, "configure GPIO for RST line failed");
    }

    switch (panel_dev_config->rgb_endian) {
    case LCD_RGB_ENDIAN_RGB:
        st7789t->madctl_val = 0;
        break;
    case LCD_RGB_ENDIAN_BGR:
        st7789t->madctl_val |= LCD_CMD_BGR_BIT;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported color space");
        break;
    }

    uint8_t fb_bits_per_pixel = 0;
    switch (panel_dev_config->bits_per_pixel) {
    case 16: // RGB565
        st7789t->colmod_cal = 0x55;
        fb_bits_per_pixel = 16;
        break;
    case 18: // RGB666
        st7789t->colmod_cal = 0x66;
        // each color component (R/G/B) should occupy the 6 high bits of a byte, which means 3 full bytes are required for a pixel
        fb_bits_per_pixel = 24;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported pixel width");
        break;
    }

    st7789t->io = io;
    st7789t->fb_bits_per_pixel = fb_bits_per_pixel;
    st7789t->reset_gpio_num = panel_dev_config->reset_gpio_num;
    st7789t->reset_level = panel_dev_config->flags.reset_active_high;
    st7789t->base.del = panel_st7789t_del;
    st7789t->base.reset = panel_st7789t_reset;
    st7789t->base.init = panel_st7789t_init;
    st7789t->base.draw_bitmap = panel_st7789t_draw_bitmap;
    st7789t->base.invert_color = panel_st7789t_invert_color;
    st7789t->base.set_gap = panel_st7789t_set_gap;
    st7789t->base.mirror = panel_st7789t_mirror;
    st7789t->base.swap_xy = panel_st7789t_swap_xy;
    st7789t->base.disp_on_off = panel_st7789t_disp_on_off;
    *ret_panel = &(st7789t->base);
    ESP_LOGD(TAG, "new st7789t panel @%p", st7789t);
    // printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\n");
    return ESP_OK;

err:
    if (st7789t) {
        if (panel_dev_config->reset_gpio_num >= 0) {
            gpio_reset_pin(panel_dev_config->reset_gpio_num);
        }
        free(st7789t);
    }
    return ret;
}

static esp_err_t panel_st7789t_del(esp_lcd_panel_t *panel)
{
    st7789t_panel_t *st7789t = __containerof(panel, st7789t_panel_t, base);

    if (st7789t->reset_gpio_num >= 0) {
        gpio_reset_pin(st7789t->reset_gpio_num);
    }
    ESP_LOGD(TAG, "del st7789t panel @%p", st7789t);
    free(st7789t);
    return ESP_OK;
}

static esp_err_t panel_st7789t_reset(esp_lcd_panel_t *panel)
{
    st7789t_panel_t *st7789t = __containerof(panel, st7789t_panel_t, base);
    esp_lcd_panel_io_handle_t io = st7789t->io;

    // perform hardware reset
    if (st7789t->reset_gpio_num >= 0) {
        gpio_set_level(st7789t->reset_gpio_num, st7789t->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(st7789t->reset_gpio_num, !st7789t->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
    } else { // perform software reset
        esp_lcd_panel_io_tx_param(io, LCD_CMD_SWRESET, NULL, 0);
        vTaskDelay(pdMS_TO_TICKS(20)); // spec, wait at least 5m before sending new command
    }

    return ESP_OK;
}

static esp_err_t panel_st7789t_init(esp_lcd_panel_t *panel)
{
    st7789t_panel_t *st7789t = __containerof(panel, st7789t_panel_t, base);
    esp_lcd_panel_io_handle_t io = st7789t->io;
    // LCD goes into sleep mode and display will be turned off after power on reset, exit sleep mode first
    // printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\n");
    esp_lcd_panel_io_tx_param(io, LCD_CMD_SLPOUT, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    // esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {st7789t->madctl_val,}, 1);
    // esp_lcd_panel_io_tx_param(io, LCD_CMD_COLMOD, (uint8_t[]) {st7789t->colmod_cal,}, 1);
    
    /* Memory Data Access Control, MX=MV=1, MY=ML=MH=0, RGB=0 */
    esp_lcd_panel_io_tx_param(io, 0x36, (uint8_t []){0x00}, 1);                           // 0x36: 接口像素格式 X镜像，Y镜像
    /* Interface Pixel Format, 16bits/pixel for RGB/MCU interface */
    esp_lcd_panel_io_tx_param(io, 0x3A, (uint8_t []){0x55}, 1);                           // 0x3A: Porch 设置
    
    esp_lcd_panel_io_tx_param(io, 0xB0, (uint8_t []){0x00, 0xE8}, 2);   
    /* Porch Setting */
    esp_lcd_panel_io_tx_param(io, 0xB2, (uint8_t []){0x0c, 0x0c, 0x00, 0x33, 0x33}, 5);      
    /* Gate Control, Vgh=13.65V, Vgl=-10.43V */
    esp_lcd_panel_io_tx_param(io, 0xB7, (uint8_t []){0x75}, 1);
    /* VCOM Setting, VCOM=1.175V */
    esp_lcd_panel_io_tx_param(io, 0xBB, (uint8_t []){0x1A}, 1);
    /* LCM Control, XOR: BGR, MX, MH */
    esp_lcd_panel_io_tx_param(io, 0xC0, (uint8_t []){0x80}, 1);
    /* VDV and VRH Command Enable, enable=1 */
    esp_lcd_panel_io_tx_param(io, 0xC2, (uint8_t []){0x01, 0xff}, 2);
    /* VRH Set, Vap=4.4+... */
    esp_lcd_panel_io_tx_param(io, 0xC3, (uint8_t []){0x13}, 1);
    /* VDV Set, VDV=0 */
    esp_lcd_panel_io_tx_param(io, 0xC4, (uint8_t []){0x20}, 1);
    /* Frame Rate Control, 60Hz, inversion=0 */
    esp_lcd_panel_io_tx_param(io, 0xC6, (uint8_t []){0x0F}, 1);
    /* Power Control 1, AVDD=6.8V, AVCL=-4.8V, VDDS=2.3V */
    esp_lcd_panel_io_tx_param(io, 0xD0, (uint8_t []){0xA4, 0xA1}, 1);
    /* Positive Voltage Gamma Control */
    esp_lcd_panel_io_tx_param(io, 0xE0, (uint8_t []){0xD0, 0x0D, 0x14, 0x0D, 0x0D, 0x09, 0x38, 0x44, 0x4E, 0x3A, 0x17, 0x18, 0x2F, 0x30}, 14);
    /* Negative Voltage Gamma Control */
    esp_lcd_panel_io_tx_param(io, 0xE1, (uint8_t []){0xD0, 0x09, 0x0F, 0x08, 0x07, 0x14, 0x37, 0x44, 0x4D, 0x38, 0x15, 0x16, 0x2C, 0x2E}, 14);
    /* Sleep Out */
    esp_lcd_panel_io_tx_param(io, 0x21, NULL, 0);
    /* Display On */
    esp_lcd_panel_io_tx_param(io, 0x29, NULL, 0);

    esp_lcd_panel_io_tx_param(io, 0x2C, NULL, 0);

    return ESP_OK;
}

static esp_err_t panel_st7789t_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    st7789t_panel_t *st7789t = __containerof(panel, st7789t_panel_t, base);
    assert((x_start < x_end) && (y_start < y_end) && "start position must be smaller than end position");
    esp_lcd_panel_io_handle_t io = st7789t->io;

    x_start += st7789t->x_gap;
    x_end += st7789t->x_gap;
    y_start += st7789t->y_gap;
    y_end += st7789t->y_gap;

    // define an area of frame memory where MCU can access
    esp_lcd_panel_io_tx_param(io, LCD_CMD_CASET, (uint8_t[]) {
        (x_start >> 8) & 0xFF,
        x_start & 0xFF,
        ((x_end - 1) >> 8) & 0xFF,
        (x_end - 1) & 0xFF,
    }, 4);
    esp_lcd_panel_io_tx_param(io, LCD_CMD_RASET, (uint8_t[]) {
        (y_start >> 8) & 0xFF,
        y_start & 0xFF,
        ((y_end - 1) >> 8) & 0xFF,
        (y_end - 1) & 0xFF,
    }, 4);
    // transfer frame buffer
    size_t len = (x_end - x_start) * (y_end - y_start) * st7789t->fb_bits_per_pixel / 8;
    esp_lcd_panel_io_tx_color(io, LCD_CMD_RAMWR, color_data, len);

    return ESP_OK;
}

static esp_err_t panel_st7789t_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    st7789t_panel_t *st7789t = __containerof(panel, st7789t_panel_t, base);
    esp_lcd_panel_io_handle_t io = st7789t->io;
    int command = 0;
    if (invert_color_data) {
        command = LCD_CMD_INVON;
    } else {
        command = LCD_CMD_INVOFF;
    }
    esp_lcd_panel_io_tx_param(io, command, NULL, 0);
    return ESP_OK;
}

static esp_err_t panel_st7789t_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    st7789t_panel_t *st7789t = __containerof(panel, st7789t_panel_t, base);
    esp_lcd_panel_io_handle_t io = st7789t->io;
    if (mirror_x) {
        st7789t->madctl_val |= LCD_CMD_MX_BIT;
    } else {
        st7789t->madctl_val &= ~LCD_CMD_MX_BIT;
    }
    if (mirror_y) {
        st7789t->madctl_val |= LCD_CMD_MY_BIT;
    } else {
        st7789t->madctl_val &= ~LCD_CMD_MY_BIT;
    }
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
        st7789t->madctl_val
    }, 1);
    return ESP_OK;
}

static esp_err_t panel_st7789t_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    st7789t_panel_t *st7789t = __containerof(panel, st7789t_panel_t, base);
    esp_lcd_panel_io_handle_t io = st7789t->io;
    if (swap_axes) {
        st7789t->madctl_val |= LCD_CMD_MV_BIT;
    } else {
        st7789t->madctl_val &= ~LCD_CMD_MV_BIT;
    }
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
        st7789t->madctl_val
    }, 1);
    return ESP_OK;
}

static esp_err_t panel_st7789t_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    st7789t_panel_t *st7789t = __containerof(panel, st7789t_panel_t, base);
    st7789t->x_gap = x_gap;
    st7789t->y_gap = y_gap;
    return ESP_OK;
}

static esp_err_t panel_st7789t_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    st7789t_panel_t *st7789t = __containerof(panel, st7789t_panel_t, base);
    esp_lcd_panel_io_handle_t io = st7789t->io;
    int command = 0;
    if (on_off) {
        command = LCD_CMD_DISPON;
    } else {
        command = LCD_CMD_DISPOFF;
    }
    esp_lcd_panel_io_tx_param(io, command, NULL, 0);
    return ESP_OK;
}
