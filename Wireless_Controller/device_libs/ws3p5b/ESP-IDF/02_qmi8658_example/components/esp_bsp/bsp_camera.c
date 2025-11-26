
#include "esp_camera.h"
#include "driver/i2c_master.h"


#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 38
#define SIOD_GPIO_NUM 8
#define SIOC_GPIO_NUM 7

#define Y9_GPIO_NUM 21
#define Y8_GPIO_NUM 39
#define Y7_GPIO_NUM 40
#define Y6_GPIO_NUM 42
#define Y5_GPIO_NUM 46
#define Y4_GPIO_NUM 48
#define Y3_GPIO_NUM 47
#define Y2_GPIO_NUM 45
#define VSYNC_GPIO_NUM 17
#define HREF_GPIO_NUM 18
#define PCLK_GPIO_NUM 41

#define CAM_LEDC_TIMER      LEDC_TIMER_1
#define CAM_LEDC_CHANNEL    LEDC_CHANNEL_0

void bsp_camera_init(i2c_port_num_t i2c_port) 
{
    camera_config_t config;
    config.ledc_channel = CAM_LEDC_CHANNEL;
    config.ledc_timer = CAM_LEDC_TIMER;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = -1;
    config.pin_sccb_scl = -1;
    config.sccb_i2c_port = 0;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_320X480;
    config.pixel_format = PIXFORMAT_RGB565; // for streaming
    // config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    esp_err_t err = esp_camera_init(&config);
    

    if (err != ESP_OK)
    {
        printf("Camera init failed with error 0x%x", err);
        return;
    }
    sensor_t *s = esp_camera_sensor_get();
    s->set_vflip(s, 1);
}