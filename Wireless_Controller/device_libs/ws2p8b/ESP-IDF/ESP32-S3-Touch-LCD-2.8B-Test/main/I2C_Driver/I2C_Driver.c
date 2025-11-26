#include "I2C_Driver.h"


#define I2C_TRANS_BUF_MINIMUM_SIZE     (sizeof(i2c_cmd_desc_t) + \
                                        sizeof(i2c_cmd_link_t) * 8) /* It is required to have allocate one i2c_cmd_desc_t per command:
                                                                     * start + write (device address) + write buffer +
                                                                     * start + write (device address) + read buffer + read buffer for NACK +
                                                                     * stop */
static const char *I2C_TAG = "I2C";
/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_Touch_SDA_IO,
        .scl_io_num = I2C_Touch_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}
void I2C_Init(void)
{
    /********************* I2C *********************/
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(I2C_TAG, "I2C initialized successfully");  
}


// Reg addr is 8 bit
esp_err_t I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length)
{
    uint8_t buf[Length+1];
    
    buf[0] = Reg_addr;
    // Copy Reg_data to buf starting at buf[1]
    memcpy(&buf[1], Reg_data, Length);
    return i2c_master_write_to_device(I2C_MASTER_NUM, Driver_addr, buf, Length+1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}



esp_err_t I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, Driver_addr, &Reg_addr, 1, Reg_data, Length, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}
