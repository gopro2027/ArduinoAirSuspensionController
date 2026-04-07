/*
 * SPDX-FileCopyrightText: 2016-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modified by planevina 2025-01-20
 */

#include <stdio.h>
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "bidi_switch_knob.h"

static const char *TAG = "Knob";

#define TICKS_INTERVAL 3
#define DEBOUNCE_TICKS 2

#define KNOB_CHECK(a, str, ret_val)                               \
    if (!(a))                                                     \
    {                                                             \
        ESP_LOGE(TAG, "%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val);                                         \
    }

#define KNOB_CHECK_GOTO(a, str, label)                                         \
    if (!(a))                                                                  \
    {                                                                          \
        ESP_LOGE(TAG, "%s:%d (%s):%s", __FILE__, __LINE__, __FUNCTION__, str); \
        goto label;                                                            \
    }

#define CALL_EVENT_CB(ev) \
    if (knob->cb[ev])     \
    knob->cb[ev](knob, knob->usr_data[ev])

typedef struct Knob
{
    bool encoder_a_change;
    bool encoder_b_change;
    uint8_t debounce_a_cnt;
    uint8_t debounce_b_cnt;
    uint8_t encoder_a_level;
    uint8_t encoder_b_level;
    knob_event_t event;
    int count_value;
    uint8_t (*hal_knob_level)(void *hardware_data);
    void *encoder_a;
    void *encoder_b;
    void *usr_data[KNOB_EVENT_MAX];
    knob_cb_t cb[KNOB_EVENT_MAX];
    struct Knob *next;
} knob_dev_t;

static knob_dev_t *s_head_handle = NULL;
static esp_timer_handle_t s_knob_timer_handle;
static bool s_is_timer_running = false;

static void process_knob_channel(uint8_t current_level, uint8_t *prev_level,
                                 uint8_t *debounce_cnt, int *count_value,
                                 knob_event_t event, bool is_increment, knob_dev_t *knob)
{
    if (current_level == 0)
    {
        if (current_level != *prev_level)
            *debounce_cnt = 0;
        else
            (*debounce_cnt)++;
    }
    else
    {
        if (current_level != *prev_level && ++(*debounce_cnt) >= DEBOUNCE_TICKS)
        {
            *debounce_cnt = 0;
            *count_value += is_increment ? 1 : -1;
            knob->event = event;
            CALL_EVENT_CB(event);
        }
        else
            *debounce_cnt = 0;
    }
    *prev_level = current_level;
}

static void knob_handler(knob_dev_t *knob)
{
    uint8_t pha_value = knob->hal_knob_level(knob->encoder_a);
    uint8_t phb_value = knob->hal_knob_level(knob->encoder_b);

    process_knob_channel(pha_value, &knob->encoder_a_level,
                         &knob->debounce_a_cnt, &knob->count_value,
                         KNOB_RIGHT, true, knob);

    process_knob_channel(phb_value, &knob->encoder_b_level,
                         &knob->debounce_b_cnt, &knob->count_value,
                         KNOB_LEFT, false, knob);
}

static void knob_cb(void *args)
{
    knob_dev_t *target;
    for (target = s_head_handle; target; target = target->next)
    {
        knob_handler(target);
    }
}

knob_handle_t iot_knob_create(const knob_config_t *config)
{
    KNOB_CHECK(NULL != config, "config pointer can't be NULL!", NULL)
    KNOB_CHECK(config->gpio_encoder_a != config->gpio_encoder_b, "encoder A can't be the same as encoder B", NULL);

    knob_dev_t *knob = (knob_dev_t *)calloc(1, sizeof(knob_dev_t));
    KNOB_CHECK(NULL != knob, "alloc knob failed", NULL);

    esp_err_t ret = ESP_OK;
    ret = knob_gpio_init(config->gpio_encoder_a);
    KNOB_CHECK(ESP_OK == ret, "encoder A gpio init failed", NULL);
    ret = knob_gpio_init(config->gpio_encoder_b);
    KNOB_CHECK_GOTO(ESP_OK == ret, "encoder B gpio init failed", _encoder_deinit);

    knob->hal_knob_level = knob_gpio_get_key_level;
    knob->encoder_a = (void *)(long)config->gpio_encoder_a;
    knob->encoder_b = (void *)(long)config->gpio_encoder_b;

    knob->encoder_a_level = knob->hal_knob_level(knob->encoder_a);
    knob->encoder_b_level = knob->hal_knob_level(knob->encoder_b);

    knob->event = KNOB_NONE;

    knob->next = s_head_handle;
    s_head_handle = knob;

    if (!s_knob_timer_handle)
    {
        esp_timer_create_args_t knob_timer = {0};
        knob_timer.arg = NULL;
        knob_timer.callback = knob_cb;
        knob_timer.dispatch_method = ESP_TIMER_TASK;
        knob_timer.name = "knob_timer";
        esp_timer_create(&knob_timer, &s_knob_timer_handle);
    }

    if (!s_is_timer_running)
    {
        esp_timer_start_periodic(s_knob_timer_handle, TICKS_INTERVAL * 1000U);
        s_is_timer_running = true;
    }

    ESP_LOGI(TAG, "Iot Knob Config Succeed, encoder A:%d, encoder B:%d", config->gpio_encoder_a, config->gpio_encoder_b);
    return (knob_handle_t)knob;

_encoder_deinit:
    knob_gpio_deinit(config->gpio_encoder_b);
    knob_gpio_deinit(config->gpio_encoder_a);
    return NULL;
}

esp_err_t iot_knob_delete(knob_handle_t knob_handle)
{
    esp_err_t ret = ESP_OK;
    KNOB_CHECK(NULL != knob_handle, "Pointer of handle is invalid", ESP_ERR_INVALID_ARG);
    knob_dev_t *knob = (knob_dev_t *)knob_handle;
    ret = knob_gpio_deinit((int)((intptr_t)knob->encoder_a));
    KNOB_CHECK(ESP_OK == ret, "knob deinit failed", ESP_FAIL);
    knob_dev_t **curr;
    for (curr = &s_head_handle; *curr;)
    {
        knob_dev_t *entry = *curr;
        if (entry == knob)
        {
            *curr = entry->next;
            free(entry);
        }
        else
        {
            curr = &entry->next;
        }
    }

    uint16_t number = 0;
    knob_dev_t *target = s_head_handle;
    while (target)
    {
        target = target->next;
        number++;
    }
    ESP_LOGD(TAG, "remain knob number=%d", number);

    if (0 == number && s_is_timer_running)
    {
        esp_timer_stop(s_knob_timer_handle);
        esp_timer_delete(s_knob_timer_handle);
        s_is_timer_running = false;
    }

    return ESP_OK;
}

esp_err_t iot_knob_register_cb(knob_handle_t knob_handle, knob_event_t event, knob_cb_t cb, void *usr_data)
{
    KNOB_CHECK(NULL != knob_handle, "Pointer of handle is invalid", ESP_ERR_INVALID_ARG);
    KNOB_CHECK(event < KNOB_EVENT_MAX, "event is invalid", ESP_ERR_INVALID_ARG);
    knob_dev_t *knob = (knob_dev_t *)knob_handle;
    knob->cb[event] = cb;
    knob->usr_data[event] = usr_data;
    return ESP_OK;
}

esp_err_t iot_knob_unregister_cb(knob_handle_t knob_handle, knob_event_t event)
{
    KNOB_CHECK(NULL != knob_handle, "Pointer of handle is invalid", ESP_ERR_INVALID_ARG);
    KNOB_CHECK(event < KNOB_EVENT_MAX, "event is invalid", ESP_ERR_INVALID_ARG);
    knob_dev_t *knob = (knob_dev_t *)knob_handle;
    knob->cb[event] = NULL;
    knob->usr_data[event] = NULL;
    return ESP_OK;
}

knob_event_t iot_knob_get_event(knob_handle_t knob_handle)
{
    KNOB_CHECK(NULL != knob_handle, "Pointer of handle is invalid", ESP_ERR_INVALID_ARG);
    knob_dev_t *knob = (knob_dev_t *)knob_handle;
    return knob->event;
}

int iot_knob_get_count_value(knob_handle_t knob_handle)
{
    KNOB_CHECK(NULL != knob_handle, "Pointer of handle is invalid", ESP_ERR_INVALID_ARG);
    knob_dev_t *knob = (knob_dev_t *)knob_handle;
    return knob->count_value;
}

esp_err_t iot_knob_clear_count_value(knob_handle_t knob_handle)
{
    KNOB_CHECK(NULL != knob_handle, "Pointer of handle is invalid", ESP_ERR_INVALID_ARG);
    knob_dev_t *knob = (knob_dev_t *)knob_handle;
    knob->count_value = 0;
    return ESP_OK;
}

esp_err_t iot_knob_resume(void)
{
    KNOB_CHECK(s_knob_timer_handle, "knob timer handle is invalid", ESP_ERR_INVALID_STATE);
    KNOB_CHECK(!s_is_timer_running, "knob timer is already running", ESP_ERR_INVALID_STATE);

    esp_err_t err = esp_timer_start_periodic(s_knob_timer_handle, TICKS_INTERVAL * 1000U);
    KNOB_CHECK(ESP_OK == err, "knob timer start failed", ESP_FAIL);
    s_is_timer_running = true;
    return ESP_OK;
}

esp_err_t iot_knob_stop(void)
{
    KNOB_CHECK(s_knob_timer_handle, "knob timer handle is invalid", ESP_ERR_INVALID_STATE);
    KNOB_CHECK(s_is_timer_running, "knob timer is not running", ESP_ERR_INVALID_STATE);

    esp_err_t err = esp_timer_stop(s_knob_timer_handle);
    KNOB_CHECK(ESP_OK == err, "knob timer stop failed", ESP_FAIL);
    s_is_timer_running = false;
    return ESP_OK;
}

esp_err_t knob_gpio_init(uint32_t gpio_num)
{
    gpio_config_t gpio_cfg = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up_en = 1,
    };
    esp_err_t ret = gpio_config(&gpio_cfg);
    return ret;
}

esp_err_t knob_gpio_deinit(uint32_t gpio_num)
{
    return gpio_reset_pin(gpio_num);
}

uint8_t knob_gpio_get_key_level(void *gpio_num)
{
    return (uint8_t)gpio_get_level((uint32_t)((intptr_t)gpio_num));
}
