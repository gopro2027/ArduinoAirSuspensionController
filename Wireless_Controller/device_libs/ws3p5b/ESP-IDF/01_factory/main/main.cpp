#include <stdio.h>

#include "nvs_flash.h"
#include "nvs.h"

#include "bsp_i2c.h"
#include "bsp_qmi8658.h"
#include "bsp_pcf85063.h"
#include "bsp_display.h"
#include "bsp_touch.h"
#include "bsp_sdcard.h"
#include "bsp_wifi.h"
#include "bsp_camera.h"
#include "bsp_es8311.h"
#include "bsp_axp2101.h"

#include "lv_port.h"

#include "demos/lv_demos.h"
#include "drawing_screen.h"
#include "esp_io_expander_tca9554.h"

#include "lvgl_ui.h"
#include "iot_button.h"
#include "button_gpio.h"

#define EXAMPLE_DISPLAY_ROTATION LV_DISP_ROT_NONE
#define EXAMPLE_LCD_H_RES 320
#define EXAMPLE_LCD_V_RES 480
#define LCD_BUFFER_SIZE EXAMPLE_LCD_H_RES *EXAMPLE_LCD_V_RES

esp_io_expander_handle_t expander_handle = NULL;

esp_lcd_panel_io_handle_t io_handle = NULL;
esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_touch_handle_t touch_handle = NULL;
lv_disp_drv_t disp_drv;

static lv_disp_t *lvgl_disp;
static lv_indev_t *lvgl_touch_indev = NULL;

void button_init(void);
void touch_test(void);
void lv_port_init(void);

void io_expander_init(i2c_master_bus_handle_t bus_handle)
{
    ESP_ERROR_CHECK(esp_io_expander_new_i2c_tca9554(bus_handle, ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000, &expander_handle));
    ESP_ERROR_CHECK(esp_io_expander_set_dir(expander_handle, IO_EXPANDER_PIN_NUM_1, IO_EXPANDER_OUTPUT));
    // ESP_ERROR_CHECK(esp_io_expander_set_level(expander_handle, IO_EXPANDER_PIN_NUM_1, 1));
    // vTaskDelay(pdMS_TO_TICKS(10));
    ESP_ERROR_CHECK(esp_io_expander_set_level(expander_handle, IO_EXPANDER_PIN_NUM_1, 0));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(esp_io_expander_set_level(expander_handle, IO_EXPANDER_PIN_NUM_1, 1));
    vTaskDelay(pdMS_TO_TICKS(200));
}
bool touch_test_done = false;
extern "C" void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    i2c_master_bus_handle_t i2c_bus_handle = bsp_i2c_init();

    bsp_axp2101_init(i2c_bus_handle);
    bsp_qmi8658_init(i2c_bus_handle);
    // bsp_qmi8658_test();

    bsp_pcf85063_init(i2c_bus_handle);
    // bsp_pcf85063_test();
    io_expander_init(i2c_bus_handle);
    bsp_display_init(&io_handle, &panel_handle, LCD_BUFFER_SIZE);
    bsp_touch_init(i2c_bus_handle, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, 0);
    bsp_camera_init(0);
    bsp_wifi_init("WSTEST", "waveshare0755");
    bsp_es8311_init(i2c_bus_handle);
    bsp_sdcard_init();
    bsp_display_brightness_init();
    bsp_display_set_brightness(100);

    lv_port_init();

    // button_init();
    // touch_test();


    
    if (lvgl_port_lock(0))
    {
        drawing_screen_init();
        lvgl_port_unlock();
    }
    while (!canvas_exit)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    
    if (lvgl_port_lock(0))
    {
        lv_obj_del(canvas);
        // drawing_screen_init();
        // lv_demo_benchmark();
        // lv_demo_music();
        // lv_demo_widgets();
        lvgl_ui_init();
        lvgl_port_unlock();
    }
}

static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    static lv_coord_t last_x = 0;
    static lv_coord_t last_y = 0;
    touch_data_t touch_data;
    /*Save the pressed coordinates and the state*/
    bsp_touch_read();
    if (bsp_touch_get_coordinates(&touch_data))
    {
        last_x = touch_data.coords[0].x;
        last_y = touch_data.coords[0].y;
        data->state = LV_INDEV_STATE_PR;
        // printf("x: %d, y: %d\n", last_x, last_y);
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
    /*Set the last pressed coordinates*/
    data->point.x = last_x;
    data->point.y = last_y;
}

void lv_port_init(void)
{
    lvgl_port_cfg_t port_cfg = {};

    port_cfg.task_priority = 4;
    port_cfg.task_stack = 1024 * 5;
    port_cfg.task_affinity = 1;
    port_cfg.task_max_sleep_ms = 500;
    port_cfg.timer_period_ms = 5;
    lvgl_port_init(&port_cfg);

    lvgl_port_display_cfg_t disp_cfg = {};
    disp_cfg.io_handle = io_handle;
    disp_cfg.panel_handle = panel_handle;
    disp_cfg.buffer_size = LCD_BUFFER_SIZE;
    disp_cfg.sw_rotate = EXAMPLE_DISPLAY_ROTATION;
    disp_cfg.hres = EXAMPLE_LCD_H_RES;
    disp_cfg.vres = EXAMPLE_LCD_V_RES;
    disp_cfg.trans_size = LCD_BUFFER_SIZE / 10;
    disp_cfg.draw_wait_cb = NULL;
    disp_cfg.flags.buff_dma = false;
    disp_cfg.flags.buff_spiram = true;

    if (disp_cfg.sw_rotate == LV_DISP_ROT_180 || disp_cfg.sw_rotate == LV_DISP_ROT_NONE)
    {
        disp_cfg.hres = EXAMPLE_LCD_H_RES;
        disp_cfg.vres = EXAMPLE_LCD_V_RES;
    }
    else
    {
        disp_cfg.hres = EXAMPLE_LCD_V_RES;
        disp_cfg.vres = EXAMPLE_LCD_H_RES;
    }
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);

    // lvgl_port_touch_cfg_t touch_cfg = {};
    // touch_cfg.disp = lvgl_disp;
    // touch_cfg.handle = touch_handle;
    // touch_cfg.touch_wait_cb = NULL;

    // lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lvgl_touch_indev = lv_indev_drv_register(&indev_drv);
}


static void button_event_cb(void *arg, void *data)
{
    button_event_t event = iot_button_get_event((button_handle_t)arg);
    // ESP_LOGI(TAG, "%s", iot_button_get_event_str(event));
    touch_test_done = true;
}

void button_init(void)
{
    button_config_t btn_cfg = {};
    button_gpio_config_t btn_gpio_cfg = {};
    btn_gpio_cfg.gpio_num = GPIO_NUM_0;
    btn_gpio_cfg.active_level = 0;
    static button_handle_t btn = NULL;
    ESP_ERROR_CHECK(iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &btn));
    iot_button_register_cb(btn, BUTTON_SINGLE_CLICK, NULL, button_event_cb, NULL);
    // iot_button_register_cb(btn, BUTTON_LONG_PRESS_START, NULL, button_event_cb, NULL);
    // iot_button_register_cb(btn, BUTTON_LONG_PRESS_HOLD, NULL, button_event_cb, NULL);
    // iot_button_register_cb(btn, BUTTON_LONG_PRESS_UP, NULL, button_event_cb, NULL);
    // iot_button_register_cb(btn, BUTTON_PRESS_END, NULL, button_event_cb, NULL);
}

void touch_test(void)
{
    uint16_t touchpad_x[1] = {0};
    uint16_t touchpad_y[1] = {0};
    uint8_t touchpad_cnt = 0;
    uint16_t color_arr[16] = {0};
    lv_obj_t *lable = NULL;

    for (int i = 0; i < 16; i++)
    {
        color_arr[i] = 0xf800;
    }
    if (lvgl_port_lock(0))
    {
        lable = lv_label_create(lv_scr_act());
        lv_label_set_text(lable, "Touch testing mode \nExit with BOOT button");
        lv_obj_center(lable);
        lvgl_port_unlock();
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    touch_data_t touch_data;
    uint16_t *lcd_buffer = (uint16_t *)heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (lvgl_port_lock(0))
    {

        while (!touch_test_done)
        {
            /* Read data from touch controller into memory */
            bsp_touch_read();

            /* Read data from touch controller */
            if (bsp_touch_get_coordinates(&touch_data))
            {
                // touchpad_x[0] = EXAMPLE_LCD_H_RES - 1 - touchpad_x[0];

                if (touch_data.coords[0].x < 2)
                    touch_data.coords[0].x = 2;
                else if (touch_data.coords[0].x > EXAMPLE_LCD_H_RES - 2 - 1)
                    touch_data.coords[0].x = EXAMPLE_LCD_H_RES - 2 - 1;

                if (touch_data.coords[0].y < 2)
                    touch_data.coords[0].y = 2;
                else if (touch_data.coords[0].y > EXAMPLE_LCD_V_RES - 2 - 1)
                    touch_data.coords[0].y = EXAMPLE_LCD_V_RES - 2 - 1;

                for (int i = 0; i < 2; i++)
                {
                    for (int j = 0; j < 2; j++)
                    {
                        lcd_buffer[(touch_data.coords[0].y + j) * EXAMPLE_LCD_H_RES + touch_data.coords[0].x + i] = 0xF800;
                    }
                }
                esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, lcd_buffer);
                // esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES - 1, EXAMPLE_LCD_V_RES - 1, lcd_buffer);
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        heap_caps_free(lcd_buffer);
        lv_obj_del(lable);
        lvgl_port_unlock();
    }
}