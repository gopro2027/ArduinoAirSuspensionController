
#include "camera_tile.h"
#include "esp_camera.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "lv_port.h"

lv_obj_t *cam_ing;

void camera_task(void *arg)
{
    camera_fb_t *pic;
    lv_img_dsc_t img_dsc;
    img_dsc.header.always_zero = 0;
    img_dsc.header.w = 320;
    img_dsc.header.h = 480;
    img_dsc.data_size = 320 * 480 * 2;
    img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    img_dsc.data = NULL;
    while (1)
    {
        pic = esp_camera_fb_get();

        if (NULL != pic)
        {
            // printf("pic->len = %d\n", pic->len);
            img_dsc.data = pic->buf;
            if (lvgl_port_lock(0))
            {
                lv_img_set_src(cam_ing, &img_dsc);
                lvgl_port_unlock();
            }
        }
        esp_camera_fb_return(pic);

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void camera_tile_init(lv_obj_t *parent)
{
    cam_ing = lv_img_create(parent);
    lv_obj_set_size(cam_ing, lv_pct(100), lv_pct(100));
    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL)
    {
        xTaskCreatePinnedToCore(camera_task, "camera_task", 1024 * 2, NULL, 1, NULL, 1);
    }
}