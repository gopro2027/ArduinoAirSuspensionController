#pragma once

#include "Arduino.h"
#include "I2C_Driver.h"
#include "TCA9554PWR.h"
#include "Display_ST7701.h"  

#define GT911_ADDR          0x5D
#define GT911_INT_PIN       16
     
#define Mirror_X       0                               
#define Mirror_Y       0

#define Touch_WIDTH     ESP_PANEL_LCD_WIDTH
#define Touch_HEIGHT    ESP_PANEL_LCD_HEIGHT

#define GT911_LCD_TOUCH_MAX_POINTS             (5)      
/* GT911 registers */
#define ESP_LCD_TOUCH_GT911_PRODUCT_ID_REG    (0x8140)
#define ESP_LCD_TOUCH_GT911_Resolution_REG    (0x8146)
#define ESP_LCD_TOUCH_GT911_READ_DATA_REG     (0x814E)


extern uint8_t Touch_interrupts;

struct GT911_Touch{
  uint8_t points;    // Number of touch points
  struct {
    uint16_t x; /*!< X coordinate */
    uint16_t y; /*!< Y coordinate */
    uint16_t strength; /*!< Strength */
  }coords[GT911_LCD_TOUCH_MAX_POINTS];
};


uint8_t Touch_Init();
void Touch_Loop(void);
uint8_t GT911_Touch_Reset(void);
void GT911_Read_cfg(void);
uint8_t Touch_Read_Data(void);
uint8_t Touch_Get_XY(uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num);
void example_touchpad_read(void);
void IRAM_ATTR Touch_GT911_ISR(void);
