#include "I2C_Driver.h"

static bool i2c_started = false;

void I2C_Init(void)
{
  if (i2c_started)
    return;
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_Frequency);
  i2c_started = true;
}

// 8-bit register address devices
bool I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length)
{
  Wire.beginTransmission(Driver_addr);
  Wire.write(Reg_addr);
  if (Wire.endTransmission(true)) {
    printf("The I2C transmission fails. - I2C Read\r\n");
    return false;
  }
  Wire.requestFrom(Driver_addr, Length);
  for (uint32_t i = 0; i < Length; i++) {
    *Reg_data++ = Wire.read();
  }
  return true;
}

bool I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length)
{
  Wire.beginTransmission(Driver_addr);
  Wire.write(Reg_addr);
  for (uint32_t i = 0; i < Length; i++) {
    Wire.write(*Reg_data++);
  }
  if (Wire.endTransmission(true)) {
    printf("The I2C transmission fails. - I2C Write\r\n");
    return false;
  }
  return true;
}
