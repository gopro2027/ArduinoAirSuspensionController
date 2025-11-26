#ifndef __BSP_TOUCH_H__
#define __BSP_TOUCH_H__

#include "driver/i2c_master.h"
#include "esp_lcd_axs15231b.h"


#define EXAMPLE_PIN_TP_INT GPIO_NUM_NC
#define EXAMPLE_PIN_TP_RST GPIO_NUM_NC

#define MAX_TOUCH_MAX_POINTS    2

#define I2C_AXS15231B_ADDRESS    (0x3B)

typedef struct{
    uint16_t x;
    uint16_t y;
}coords_t;

typedef struct{
    coords_t coords[MAX_TOUCH_MAX_POINTS];
    uint8_t touch_num;
}touch_data_t;



#ifdef __cplusplus
extern "C" {
#endif
// void bsp_touch_init(esp_lcd_touch_handle_t *touch_handle, i2c_master_bus_handle_t bus_handle, uint16_t xmax, uint16_t ymax, uint16_t rotation);
void bsp_touch_init(i2c_master_bus_handle_t bus_handle, uint16_t width, uint16_t height, uint16_t rotation);
void bsp_touch_read(void);
bool bsp_touch_get_coordinates(touch_data_t *touch_data);
#ifdef __cplusplus
}
#endif


#endif