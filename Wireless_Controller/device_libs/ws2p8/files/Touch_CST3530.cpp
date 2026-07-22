#include "Touch_CST3530.h"

static CST3530_Touch touch2_data = {0};

static bool Touch2_I2C_Probe(uint8_t driver_addr)
{
  Wire1.beginTransmission(driver_addr);
  return Wire1.endTransmission(true) == 0;
}

static bool Touch2_I2C_Read(uint8_t driver_addr, uint32_t reg_addr, uint8_t *reg_data, uint32_t length)
{
  Wire1.beginTransmission(driver_addr);
  Wire1.write((uint8_t)((reg_addr >> 24) & 0xFF));
  Wire1.write((uint8_t)((reg_addr >> 16) & 0xFF));
  Wire1.write((uint8_t)((reg_addr >> 8) & 0xFF));
  Wire1.write((uint8_t)(reg_addr & 0xFF));
  if (Wire1.endTransmission(false)) {
    printf("The I2C transmission fails. - CST3530 I2C Read\r\n");
    return false;
  }

  uint32_t read_len = Wire1.requestFrom(driver_addr, length);
  if (read_len != length) {
    printf("The I2C read length is wrong. - CST3530 I2C Read\r\n");
    return false;
  }

  for (uint32_t i = 0; i < length; i++) {
    *reg_data++ = Wire1.read();
  }
  return true;
}

static bool Touch2_I2C_Write(uint8_t driver_addr, uint32_t reg_addr, const uint8_t *reg_data, uint32_t length)
{
  Wire1.beginTransmission(driver_addr);
  Wire1.write((uint8_t)((reg_addr >> 24) & 0xFF));
  Wire1.write((uint8_t)((reg_addr >> 16) & 0xFF));
  Wire1.write((uint8_t)((reg_addr >> 8) & 0xFF));
  Wire1.write((uint8_t)(reg_addr & 0xFF));
  for (uint32_t i = 0; i < length; i++) {
    Wire1.write(*reg_data++);
  }
  if (Wire1.endTransmission(true)) {
    printf("The I2C transmission fails. - CST3530 I2C Write\r\n");
    return false;
  }
  return true;
}

uint8_t TOUCH2_Init(void)
{
  Wire1.begin(CST3530_SDA_PIN, CST3530_SCL_PIN, CST3530_I2C_FREQ_HZ);
  pinMode(CST3530_INT_PIN, INPUT);
  pinMode(CST3530_RST_PIN, OUTPUT);

  CST3530_Touch_Reset();
  if (!Touch2_I2C_Probe(CST3530_ADDR)) {
    printf("CST3530 not found at I2C address 0x%02X\r\n", CST3530_ADDR);
    return false;
  }

  printf("Touch controller CST3530 initialized\r\n");
  return true;
}

uint8_t CST3530_Touch_Reset(void)
{
  digitalWrite(CST3530_RST_PIN, LOW);
  delay(100);
  digitalWrite(CST3530_RST_PIN, HIGH);
  delay(500);
  return true;
}

uint8_t Touch2_Read_Data(void)
{
  uint8_t buf[51] = {0};
  uint8_t touch_cnt = 0;

  noInterrupts();
  touch2_data.points = 0;
  interrupts();

  if (!Touch2_I2C_Read(CST3530_ADDR, ESP_LCD_TOUCH_CST3530_DATA_REG, buf, 9)) {
    return false;
  }

  if ((buf[3] & 0x0F) == 0x00 || (buf[8] & 0xF0) == 0x00) {
    Touch2_I2C_Write(CST3530_ADDR, ESP_LCD_TOUCH_CST3530_END_READ_REG, NULL, 0);
    return true;
  }

  touch_cnt = buf[3] & 0x0F;
  if (touch_cnt > CST3530_LCD_TOUCH_MAX_POINTS || touch_cnt == 0) {
    Touch2_I2C_Write(CST3530_ADDR, ESP_LCD_TOUCH_CST3530_END_READ_REG, NULL, 0);
    return true;
  }

  if (touch_cnt > 1) {
    if (!Touch2_I2C_Read(CST3530_ADDR, ESP_LCD_TOUCH_CST3530_COORD_NEXT_REG, &buf[9], (touch_cnt - 1) * 5)) {
      Touch2_I2C_Write(CST3530_ADDR, ESP_LCD_TOUCH_CST3530_END_READ_REG, NULL, 0);
      return false;
    }
  }

  Touch2_I2C_Write(CST3530_ADDR, ESP_LCD_TOUCH_CST3530_END_READ_REG, NULL, 0);

  noInterrupts();
  touch2_data.points = touch_cnt;
  for (size_t i = 0; i < touch_cnt; i++) {
    touch2_data.coords[i].x = (uint16_t)(((buf[(i * 5) + 7] & 0x0F) << 8) + buf[(i * 5) + 4]);
    touch2_data.coords[i].y = (uint16_t)(((buf[(i * 5) + 7] & 0xF0) << 4) + buf[(i * 5) + 5]);
    touch2_data.coords[i].strength = (uint16_t)buf[(i * 5) + 6];
  }
  interrupts();

  return true;
}

uint8_t Touch2_Get_XY(uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num)
{
  assert(x != NULL);
  assert(y != NULL);
  assert(point_num != NULL);
  assert(max_point_num > 0);

  noInterrupts();
  if (touch2_data.points > max_point_num) {
    touch2_data.points = max_point_num;
  }
  for (size_t i = 0; i < touch2_data.points; i++) {
    x[i] = touch2_data.coords[i].x;
    y[i] = touch2_data.coords[i].y;
    if (strength) {
      strength[i] = touch2_data.coords[i].strength;
    }
  }
  *point_num = touch2_data.points;
  touch2_data.points = 0;
  interrupts();

  return (*point_num > 0);
}
