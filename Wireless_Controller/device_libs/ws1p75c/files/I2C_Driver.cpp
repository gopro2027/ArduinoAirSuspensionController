#include "I2C_Driver.h"
#include "i2c_guard.h"

void I2C_Init(void) {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLK_SPEED);
    // Every device on this bus (touch, AXP2101, QMI8658) is shared with the PMIC poll task.
    i2c_guard_init(&Wire, 20);
}

bool I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length)
{
    if (!i2c_lock(10))
        return -1;
    Wire.beginTransmission(Driver_addr);
    Wire.write(Reg_addr);
    if (Wire.endTransmission(true)) {
        i2c_unlock();
        printf("The I2C transmission fails. - I2C Read\r\n");
        return -1;
    }
    Wire.requestFrom(Driver_addr, Length);
    for (int i = 0; i < Length; i++) {
        *Reg_data++ = Wire.read();
    }
    i2c_unlock();
    return 0;
}

bool I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length)
{
    if (!i2c_lock(10))
        return -1;
    Wire.beginTransmission(Driver_addr);
    Wire.write(Reg_addr);
    for (int i = 0; i < Length; i++) {
        Wire.write(*Reg_data++);
    }
    if (Wire.endTransmission(true)) {
        i2c_unlock();
        printf("The I2C transmission fails. - I2C Write\r\n");
        return -1;
    }
    i2c_unlock();
    return 0;
}
