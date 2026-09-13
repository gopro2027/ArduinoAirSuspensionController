#include "CH422G.h"

// Shadow copies. Every CH422G register is write-only over I2C (RD_IO reads the *pins*, not the
// output latch), so a read-modify-write like the TCA9554 driver does is impossible -- we have to
// remember what we last wrote.
static uint8_t ch422g_wr_set = CH422G_WR_SET_IO_OE;  // power-on default per the datasheet
static uint8_t ch422g_wr_io  = 0x00;

// A CH422G write is one raw byte to the register's own I2C address -- no register-pointer byte,
// so Wire.write() is called exactly once.
static bool CH422G_WriteReg(uint8_t reg_addr, uint8_t data)
{
  Wire.beginTransmission(reg_addr);
  Wire.write(data);
  if (Wire.endTransmission(true)) {
    printf("CH422G write failed (reg 0x%02X)\r\n", reg_addr);
    return false;
  }
  return true;
}

void CH422G_Init(void)
{
  I2C_Init();  // no-op if the bus is already up

  // IO0..IO7 as outputs.
  ch422g_wr_set |= CH422G_WR_SET_IO_OE;
  CH422G_WriteReg(CH422G_REG_WR_SET, ch422g_wr_set);

  // Starting levels. Both resets are active low and are pulsed later by LCD_Init()/Touch_Init(),
  // so park them released (high). SD_CS idles high (deselected). USB_SEL stays LOW, which is what
  // Waveshare's own examples do -- their comment: "When USB_SEL is HIGH, it enables FSUSB42UMX
  // chip and gpio19, gpio20 wired CAN_TX CAN_RX, and then don't use USB Function".
  // Backlight (EXIO2 -> DISP) stays LOW: board_driver_util.cpp only calls set_brightness(1) once
  // the splash screen has actually been flushed, and this board's panel shows noise until then.
  ch422g_wr_io = (1 << EXIO_TP_RST) | (1 << EXIO_LCD_RST) | (1 << EXIO_SD_CS);
  CH422G_WriteReg(CH422G_REG_WR_IO, ch422g_wr_io);
}

void CH422G_Set_EXIO(uint8_t pin, uint8_t state)
{
  if (pin > 7) {
    printf("CH422G_Set_EXIO: pin %u out of range (IO0..IO7)\r\n", pin);
    return;
  }
  if (state)
    ch422g_wr_io |= (uint8_t)(1 << pin);
  else
    ch422g_wr_io &= (uint8_t)~(1 << pin);

  CH422G_WriteReg(CH422G_REG_WR_IO, ch422g_wr_io);
}

uint8_t CH422G_Read_EXIOS(void)
{
  // RD_IO is a bare 1-byte read from its own address -- no register write first.
  if (Wire.requestFrom((uint8_t)CH422G_REG_RD_IO, (uint8_t)1) != 1) {
    printf("CH422G read failed\r\n");
    return 0;
  }
  return (uint8_t)Wire.read();
}

uint8_t CH422G_Read_EXIO(uint8_t pin)
{
  if (pin > 7)
    return 0;
  return (uint8_t)((CH422G_Read_EXIOS() >> pin) & 0x01);
}

void CH422G_Set_All_Input(void)
{
  ch422g_wr_set &= (uint8_t)~CH422G_WR_SET_IO_OE;
  CH422G_WriteReg(CH422G_REG_WR_SET, ch422g_wr_set);
  vTaskDelay(pdMS_TO_TICKS(2));  // datasheet: allow the direction switch to settle
}

void CH422G_Set_All_Output(void)
{
  ch422g_wr_set |= CH422G_WR_SET_IO_OE;
  CH422G_WriteReg(CH422G_REG_WR_SET, ch422g_wr_set);
}
