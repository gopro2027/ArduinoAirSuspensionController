#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bsp_i2c.h"
#include "bsp_axp2101.h"
extern "C" void app_main(void)
{
    i2c_master_bus_handle_t i2c_bus_handle = bsp_i2c_init();
    bsp_axp2101_init(i2c_bus_handle);
}
 