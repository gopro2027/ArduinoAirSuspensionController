#include "system_tile.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_flash.h"
#include "esp_psram.h"
#include "driver/temperature_sensor.h"
#include "esp_private/esp_clk.h"

#include "bsp_sdcard.h"
#include "bsp_pcf85063.h"
#include "bsp_display.h"
#include "bsp_es8311.h"

// #include "esp_pcf85063_port.h"
// #include "esp_sdcard_port.h"
// #include "esp_es8311_port.h"
// #include "esp_3inch5_lcd_port.h"

#include "lv_port.h"

SemaphoreHandle_t es8311_test_semaphore;
temperature_sensor_handle_t temp_sensor = NULL;

// extern void brightness_set_level(uint8_t level);
// extern void es8311_test_init(SemaphoreHandle_t xBinarySemaphore);

lv_obj_t *label_brightness;
lv_obj_t *label_time;
lv_obj_t *label_date;
lv_obj_t *label_flash;
lv_obj_t *label_psram;
lv_obj_t *label_chip_temp;
lv_obj_t *label_chip_freq;
lv_obj_t *label_sd;
lv_obj_t *lable_es8311_test;

static void slider_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        lv_obj_t *slider = lv_event_get_target(e);
        int value = lv_slider_get_value(slider);
        // printf("Slider value: %d\n", value);

        lv_label_set_text_fmt(label_brightness, "%d %%", value);
        bsp_display_set_brightness(value); 
        lv_event_stop_bubbling(e);
    }
}

static void system_time_cb(lv_timer_t *timer)
{
    char str[20];
    struct tm now_tm;
    if (bsp_pcf85063_get_time(&now_tm))
    {
        lv_label_set_text_fmt(label_date, "%04d-%02d-%02d", now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday);
        lv_label_set_text_fmt(label_time, "%02d:%02d:%02d", now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec);
    }

    
    float tsens_out;
    temperature_sensor_get_celsius(temp_sensor, &tsens_out);
    sprintf(str, "%.2f degrees C", tsens_out);
    lv_label_set_text(label_chip_temp, str);
}

static void lvgl_es8311_test_task(void *arg)
{
    uint8_t *data;
    const int limit_size = 5 * 48000 * 1 * (16 >> 3);
    while (1)
    {
        if (xSemaphoreTake(es8311_test_semaphore, portMAX_DELAY) == pdTRUE)
        {
            data = (uint8_t *)heap_caps_malloc(limit_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

            if (lvgl_port_lock(0))
            {
                lv_label_set_text(lable_es8311_test, "Recording");
                lvgl_port_unlock();
            }
            bsp_es8311_recording(data, limit_size);

            if (lvgl_port_lock(0))
            {
                lv_label_set_text(lable_es8311_test, "Playing");
                lvgl_port_unlock();
            }
            bsp_es8311_playing(data, limit_size);

            if (lvgl_port_lock(0))
            {
                lv_label_set_text(lable_es8311_test, "ES8311 Test");
                lvgl_port_unlock();
            }
            // printf("esp_es8311_test\r\n");
        }
    }
}

void system_init(void)
{
    uint32_t flash_size;
    uint32_t psram_size;
    uint64_t sdcard_size;
    uint32_t cpu_freq;

    es8311_test_semaphore = xSemaphoreCreateBinary();
    assert(es8311_test_semaphore != NULL);
    xTaskCreatePinnedToCore(lvgl_es8311_test_task, "lvgl_es8311_test_task", 4096, NULL, 1, NULL, 1);

    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 80);
    temperature_sensor_install(&temp_sensor_config, &temp_sensor);
    temperature_sensor_enable(temp_sensor);

    esp_flash_get_size(NULL, &flash_size);
    lv_label_set_text_fmt(label_flash, "%d MB", (int)(flash_size / 1024 / 1024));

    psram_size = (uint32_t)esp_psram_get_size();
    lv_label_set_text_fmt(label_psram, "%d MB", (int)(psram_size / 1024 / 1024));

    cpu_freq = esp_clk_cpu_freq();
    lv_label_set_text_fmt(label_chip_freq, "%d MHz", (int)(cpu_freq / 1000 / 1000));

    sdcard_size = bsp_sdcard_get_size();
    lv_label_set_text_fmt(label_sd, "%d MB", (int)(sdcard_size / 1024 / 1024));
}

static void btn_es8311_test_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED && es8311_test_semaphore != NULL)
    {
        xSemaphoreGive(es8311_test_semaphore);
    }
}

void system_tile_init(lv_obj_t *parent)
{
    /*Create a list*/
    lv_obj_t *list = lv_list_create(parent);
    lv_obj_t *lable = lv_label_create(parent);
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_label_set_text(lable, "System");
    lv_obj_align(lable, LV_ALIGN_TOP_MID, 0, 3);

    lv_obj_set_size(list, lv_pct(95), lv_pct(80));
    // lv_obj_center(list);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t *btn = lv_btn_create(parent);
    lable_es8311_test = lv_label_create(btn);
    lv_label_set_text(lable_es8311_test, "ES8311 Test");
    lv_obj_center(lable_es8311_test);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, -100, -15);
    lv_obj_add_event_cb(btn, btn_es8311_test_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *slider = lv_slider_create(parent);
    lv_slider_set_range(slider, 1, 100);
    lv_slider_set_value(slider, 80, LV_ANIM_OFF);

    lv_obj_set_size(slider, lv_pct(50), lv_pct(5));
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 55, -18);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *list_item;
    list_item = lv_list_add_btn(list, NULL, "ChipType");
    lv_obj_t *label_chip = lv_label_create(list_item);
    lv_label_set_text(label_chip, "ESP32-S3");

    list_item = lv_list_add_btn(list, NULL, "ChipTemp");
    label_chip_temp = lv_label_create(list_item);
    lv_label_set_text(label_chip_temp, "--- C");

    list_item = lv_list_add_btn(list, NULL, "ChipFreq");
    label_chip_freq = lv_label_create(list_item);
    lv_label_set_text(label_chip_freq, "--- MHz");

    list_item = lv_list_add_btn(list, NULL, "Brightness");
    label_brightness = lv_label_create(list_item);
    lv_label_set_text(label_brightness, "80 %");

    list_item = lv_list_add_btn(list, NULL, "SRAM");
    lv_obj_t *label_ram = lv_label_create(list_item);
    lv_label_set_text(label_ram, "512 KB");

    list_item = lv_list_add_btn(list, NULL, "PSRAM");
    label_psram = lv_label_create(list_item);
    lv_label_set_text(label_psram, "--- MB");

    list_item = lv_list_add_btn(list, NULL, "Flash");
    label_flash = lv_label_create(list_item);
    lv_label_set_text(label_flash, "--- MB");

    list_item = lv_list_add_btn(list, NULL, "SDCard");
    label_sd = lv_label_create(list_item);
    lv_label_set_text(label_sd, "--- MB");

    list_item = lv_list_add_btn(list, NULL, "Date");
    label_date = lv_label_create(list_item);
    lv_label_set_text(label_date, "2025-01-01");

    list_item = lv_list_add_btn(list, NULL, "Time");
    label_time = lv_label_create(list_item);
    lv_label_set_text(label_time, "12:00:00");
    system_init();
    lv_timer_create(system_time_cb, 1000, NULL);
}