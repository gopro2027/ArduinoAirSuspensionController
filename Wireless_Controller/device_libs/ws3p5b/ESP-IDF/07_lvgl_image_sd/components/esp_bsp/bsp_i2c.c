#include "bsp_i2c.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

SemaphoreHandle_t  bsp_i2c_mux;

bool bsp_i2c_lock(uint32_t timeout_ms)
{
    assert(bsp_i2c_mux && "lvgl_port_init must be called first");

    const TickType_t timeout_ticks = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(bsp_i2c_mux, timeout_ticks) == pdTRUE;
}

void bsp_i2c_unlock(void)
{
    assert(bsp_i2c_mux && "lvgl_port_init must be called first");
    xSemaphoreGiveRecursive(bsp_i2c_mux);
}


i2c_master_bus_handle_t bsp_i2c_init(void)
{
    i2c_master_bus_handle_t i2c_bus_handle;
    i2c_master_bus_config_t i2c_mst_config = {};
    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.i2c_port = (i2c_port_num_t)I2C_PORT_NUM;
    i2c_mst_config.scl_io_num = EXAMPLE_PIN_I2C_SCL;
    i2c_mst_config.sda_io_num = EXAMPLE_PIN_I2C_SDA;
    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.flags.enable_internal_pullup = 1;

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &i2c_bus_handle));

    bsp_i2c_mux = xSemaphoreCreateRecursiveMutex();
    return i2c_bus_handle;
}