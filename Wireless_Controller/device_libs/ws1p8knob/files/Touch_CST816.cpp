#include "Touch_CST816.h"
#include <Wire.h>

// Touch uses Arduino Wire only. board_drivers_init() must call I2C_Init() (Wire.begin)
// before LCD_Init() -> Touch_Init(). Mixing Wire with esp-idf i2c_driver_install() aborts.

static uint8_t I2C_read_buff(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    uint8_t err = Wire.endTransmission(false);
    if (err != 0)
        return err;
    size_t n = Wire.requestFrom((int)addr, (int)len);
    if (n != len)
        return 4;
    for (size_t i = 0; i < len; i++)
        buf[i] = Wire.read();
    return 0;
}

static uint8_t I2C_write_buff(uint8_t addr, uint8_t reg, const uint8_t *buf, uint8_t len)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    for (uint8_t i = 0; i < len; i++)
        Wire.write(buf[i]);
    return Wire.endTransmission(true);
}

void Touch_Init(void)
{
    // Same bus as I2C_Driver / board_drivers_init — Wire only, never esp-idf i2c_driver_install.
    Wire.begin(CST816_SDA_PIN, CST816_SCL_PIN);
    Wire.setClock(CST816_I2C_CLK_SPEED);

    uint8_t data = 0x00;
    I2C_write_buff(CST816_ADDR, 0x00, &data, 1);
}

uint8_t getTouch(uint16_t *x, uint16_t *y)
{
    uint8_t data[7] = {0};
    if (I2C_read_buff(CST816_ADDR, 0x00, data, 7) != 0)
        return 0;
    uint8_t count = data[2];
    if (count) {
        *x = ((uint16_t)(data[3] & 0x0F) << 8) + (uint16_t)data[4];
        *y = ((uint16_t)(data[5] & 0x0F) << 8) + (uint16_t)data[6];
        return 1;
    }
    return 0;
}
