#pragma once

#include "esp_camera.h"
#include "driver/i2c_master.h"


#ifdef __cplusplus
extern "C" {
#endif

void bsp_camera_init(i2c_port_num_t i2c_port);

#ifdef __cplusplus
}
#endif