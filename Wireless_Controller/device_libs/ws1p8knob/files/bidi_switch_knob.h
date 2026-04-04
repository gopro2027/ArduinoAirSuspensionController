/*
 * SPDX-FileCopyrightText: 2016-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modified by planevina 2025-01-20
 */

#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void (*knob_cb_t)(void *, void *);
    typedef void *knob_handle_t;

    typedef enum
    {
        KNOB_LEFT = 0,
        KNOB_RIGHT,
        KNOB_EVENT_MAX,
        KNOB_NONE,
    } knob_event_t;

    typedef struct
    {
        uint8_t gpio_encoder_a;
        uint8_t gpio_encoder_b;
    } knob_config_t;

    knob_handle_t iot_knob_create(const knob_config_t *config);
    esp_err_t iot_knob_delete(knob_handle_t knob_handle);
    esp_err_t iot_knob_register_cb(knob_handle_t knob_handle, knob_event_t event, knob_cb_t cb, void *usr_data);
    esp_err_t iot_knob_unregister_cb(knob_handle_t knob_handle, knob_event_t event);
    knob_event_t iot_knob_get_event(knob_handle_t knob_handle);
    int iot_knob_get_count_value(knob_handle_t knob_handle);
    esp_err_t iot_knob_clear_count_value(knob_handle_t knob_handle);
    esp_err_t iot_knob_resume(void);
    esp_err_t iot_knob_stop(void);
    esp_err_t knob_gpio_init(uint32_t gpio_num);
    esp_err_t knob_gpio_deinit(uint32_t gpio_num);
    uint8_t knob_gpio_get_key_level(void *gpio_num);

#ifdef __cplusplus
}
#endif
