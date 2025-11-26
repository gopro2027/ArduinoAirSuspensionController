#include <stdio.h>

#include "bsp_i2c.h"
#include "bsp_display.h"
#include "bsp_touch.h"

#include "lv_port.h"

#include "demos/lv_demos.h"
#include "esp_io_expander_tca9554.h"

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

LV_IMG_DECLARE(img_1);
LV_IMG_DECLARE(img_2);
LV_IMG_DECLARE(img_3);
LV_IMG_DECLARE(img_4);
LV_IMG_DECLARE(img_5);
static lv_obj_t *img = NULL;

static void img_gesture_event_cb(lv_event_t *e)
{
    static int img_index = 0;

    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_GESTURE)
    {
        // printf("img_gesture_event_cb\r\n");
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        // printf("dir = %d\r\n", dir);
        if (dir == LV_DIR_LEFT)
        {
            if (++img_index >= 5)
            {
                img_index = 0;
            }
        }
        else if (dir == LV_DIR_RIGHT)
        {
            if (--img_index < 0)
            {
                img_index = 4;
            }
        }
        switch (img_index)
        {
        case 0:
            lv_img_set_src(img, &img_1);
            break;
        case 1:
            lv_img_set_src(img, &img_2);
            break;
        case 2:
            lv_img_set_src(img, &img_3);
            break;
        case 3:
            lv_img_set_src(img, &img_4);
            break;
        case 4:
            lv_img_set_src(img, &img_5);
            break;
        }
        lv_indev_wait_release(lv_indev_get_act());
        // sprintf(str_buf, "0:images/%s", bin_filenames[img_index]);
        // lv_img_set_src(img, str_buf);
    }
}

extern "C" void app_main(void)
{

    i2c_master_bus_handle_t i2c_bus_handle = bsp_i2c_init();

    io_expander_init(i2c_bus_handle);
    bsp_display_init(&io_handle, &panel_handle, LCD_BUFFER_SIZE);
    bsp_touch_init(i2c_bus_handle, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, 0);
    bsp_display_brightness_init();
    bsp_display_set_brightness(100);

    lv_port_init();

    if (lvgl_port_lock(0))
    {
        img = lv_img_create(lv_scr_act());
        lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
        lv_img_set_src(img, &img_1);
        lv_obj_clear_flag(img, LV_OBJ_FLAG_SCROLLABLE); 
        lv_obj_add_flag(img, LV_OBJ_FLAG_GESTURE_BUBBLE);
        // lv_obj_add_event_cb(img, img_gesture_event_cb, LV_EVENT_ALL, NULL);
        lv_obj_add_event_cb(lv_scr_act(), img_gesture_event_cb, LV_EVENT_GESTURE, NULL);

        // lv_demo_benchmark();
        // lv_demo_music();
        // lv_demo_widgets();
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

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lvgl_touch_indev = lv_indev_drv_register(&indev_drv);
}
