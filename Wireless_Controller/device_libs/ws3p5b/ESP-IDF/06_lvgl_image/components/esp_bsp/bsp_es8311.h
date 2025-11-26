#pragma once

#include <stdio.h>
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

void bsp_es8311_init(i2c_master_bus_handle_t bus_handle);
void bsp_es8311_test(void);

void bsp_es8311_recording(uint8_t *data, size_t limit_size);
void bsp_es8311_playing(uint8_t *data, size_t limit_size);


#ifdef __cplusplus
}
#endif


