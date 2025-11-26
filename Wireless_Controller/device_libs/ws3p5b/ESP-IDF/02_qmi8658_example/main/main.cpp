#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bsp_i2c.h"
#include "bsp_qmi8658.h"
extern "C" void app_main(void)
{
    i2c_master_bus_handle_t i2c_bus_handle = bsp_i2c_init();
    bsp_qmi8658_init(i2c_bus_handle);
    bsp_qmi8658_test();
}
