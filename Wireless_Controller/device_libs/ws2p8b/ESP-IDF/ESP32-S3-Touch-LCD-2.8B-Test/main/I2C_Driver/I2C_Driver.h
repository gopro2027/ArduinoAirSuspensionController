#pragma once

#include <stdint.h>
#include <string.h>  // For memcpy
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c.h"


/********************* I2C *********************/
#define I2C_Touch_SCL_IO            7         /*!< GPIO number used for I2C master clock */
#define I2C_Touch_SDA_IO            15         /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0         /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000    /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0         /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0         /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000


void I2C_Init(void);
// Reg addr is 8 bit
esp_err_t I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length);
esp_err_t I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length);