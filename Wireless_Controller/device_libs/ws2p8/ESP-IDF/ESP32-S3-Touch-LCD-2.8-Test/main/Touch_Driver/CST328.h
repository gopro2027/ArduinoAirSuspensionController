#pragma once

#include "esp_lcd_touch/esp_lcd_touch.h"
#include "driver/i2c.h"

#include "ST7789.h"
#ifdef __cplusplus
extern "C" {
#endif


#define ESP_LCD_TOUCH_IO_I2C_CST328_ADDRESS (0x1A)
//workmode
#define CST328_REG_DEBUG_INFO_MODE              0xD101
#define CST328_REG_RESET_MODE            	    0xD102
#define CST328_REG_DEBUG_RECALIBRATION_MODE     0xD104
#define CST328_REG_DEEP_SLEEP_MODE    		    0xD105
#define CST328_REG_DEBUG_POINT_MODE	    	    0xD108
#define CST328_REG_NORMAL_MODE                  0xD109

#define CST328_REG_DEBUG_RAWDATA_MODE           0xD10A
#define CST328_REG_DEBUG_DIFF_MODE              0xD10D
#define CST328_REG_DEBUG_FACTORY_MODE           0xD119
#define CST328_REG_DEBUG_FACTORY_MODE_2         0xD120
//debug info
/****************CST328_REG_DEBUG_INFO_MODE address start***********/
#define CST328_REG_DEBUG_INFO_BOOT_TIME         0xD1FC
#define CST328_REG_DEBUG_INFO_RES_Y             0xD1FA
#define CST328_REG_DEBUG_INFO_RES_X             0xD1F8
#define CST328_REG_DEBUG_INFO_KEY_NUM           0xD1F7
#define CST328_REG_DEBUG_INFO_TP_NRX            0xD1F6
#define CST328_REG_DEBUG_INFO_TP_NTX            0xD1F4

/* CST328 registers */
#define ESP_LCD_TOUCH_CST328_READ_Number_REG    (0xD005)
#define ESP_LCD_TOUCH_CST328_READ_XY_REG        (0xD000)
#define ESP_LCD_TOUCH_CST328_READ_Checksum_REG  (0x80FF)
#define ESP_LCD_TOUCH_CST328_CONFIG_REG         (0x8047)
/**
 * @brief I2C address of the CST328 controller
 *
 */
// I2C settings
#define I2C_Touch_SCL_IO            3      /*!< GPIO number used for I2C master clock */
#define I2C_Touch_SDA_IO            1      /*!< GPIO number used for I2C master data  */
#define I2C_Touch_INT_IO            4      /*!< GPIO number used for I2C master data  */
#define I2C_Touch_RST_IO            2      /*!< GPIO number used for I2C master clock */
#define I2C_Touch_MASTER_NUM        1                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000


/**
 * @brief Touch IO configuration structure
 *
 */
#define ESP_LCD_TOUCH_IO_I2C_CST328_CONFIG()           \
    {                                       \
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_CST328_ADDRESS, \
        .control_phase_bytes = 1,           \
        .dc_bit_offset = 0,                 \
        .lcd_cmd_bits = 16,                 \
        .flags =                            \
        {                                   \
            .disable_control_phase = 1,     \
        }                                   \
    }


extern esp_lcd_touch_handle_t tp;

esp_err_t esp_lcd_touch_new_i2c_cst328(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config, esp_lcd_touch_handle_t *out_touch);

esp_err_t Touch_I2C_Init(void);
void TOUCH_Init(void);


#ifdef __cplusplus
}
#endif
