#include "Touch_GT911.h"
struct GT911_Touch touch_data = {0};
bool I2C_Read_Touch(uint8_t Driver_addr, uint16_t Reg_addr, uint8_t *Reg_data, uint32_t Length)
{
  Wire.beginTransmission(Driver_addr);
  Wire.write((uint8_t)(Reg_addr >> 8)); 
  Wire.write((uint8_t)Reg_addr);         
  if ( Wire.endTransmission(true)){
    printf("The I2C transmission fails. - I2C Read\r\n");
    return false;
  }
  Wire.requestFrom(Driver_addr, Length);
  for (int i = 0; i < Length; i++) {
    *Reg_data++ = Wire.read();
  }
  return true;
}
bool I2C_Write_Touch(uint8_t Driver_addr, uint16_t Reg_addr, const uint8_t *Reg_data, uint32_t Length)
{
  Wire.beginTransmission(Driver_addr);
  Wire.write((uint8_t)(Reg_addr >> 8));
  Wire.write((uint8_t)Reg_addr);        
  for (int i = 0; i < Length; i++) {
    Wire.write(*Reg_data++);
  }
  if ( Wire.endTransmission(true))
  {
    printf("The I2C transmission fails. - I2C Write\r\n");
    return false;
  }
  return true;
}
uint8_t Touch_Init(void) {

  pinMode(GT911_INT_PIN, OUTPUT);                  
  GT911_Touch_Reset();
  GT911_Read_cfg();

  attachInterrupt(GT911_INT_PIN, Touch_GT911_ISR, FALLING); 

  return true;
}
/* Reset controller */
uint8_t GT911_Touch_Reset(void)
{
  pinMode(GT911_INT_PIN, OUTPUT);                     
  digitalWrite(GT911_INT_PIN, LOW);                   

  Set_EXIO(EXIO_PIN2,LOW);
  vTaskDelay(pdMS_TO_TICKS(10));

  Set_EXIO(EXIO_PIN2,High);
  vTaskDelay(pdMS_TO_TICKS(200));

  digitalWrite(GT911_INT_PIN, HIGH);               
  pinMode(GT911_INT_PIN, INPUT);                   

  return true;
}

uint16_t X_MAX = 0;
uint16_t Y_MAX = 0;
void GT911_Read_cfg(void) {
  uint8_t buf[4];

  I2C_Read_Touch(GT911_ADDR, ESP_LCD_TOUCH_GT911_PRODUCT_ID_REG, buf, 4);
  printf("TouchPad_ID:0x%02x,0x%02x,0x%02x,0x%02x\r\n", buf[0], buf[1], buf[2], buf[3]);
  I2C_Read_Touch(GT911_ADDR, ESP_LCD_TOUCH_GT911_Resolution_REG, buf, 4);
  X_MAX = (uint16_t)(((uint16_t)buf[1] << 8) + buf[0]);
  Y_MAX = (uint16_t)(((uint16_t)buf[3] << 8) + buf[2]);
  printf("TouchPad: X_MAX:%d  Y_MAX:%d  \r\n", X_MAX, Y_MAX);
}

// reads sensor and touches
// updates Touch Points, but if not touched, resets all Touch Point Information
uint8_t Touch_Read_Data(void) {
  uint8_t buf[41];
  uint8_t touch_cnt = 0;
  uint8_t clear = 0;
  uint8_t Over = 0xAB;
  size_t i = 0,num=0;
  I2C_Read_Touch(GT911_ADDR, ESP_LCD_TOUCH_GT911_READ_DATA_REG, buf, 1);
  if ((buf[0] & 0x80) == 0x00) {                                              
    I2C_Write_Touch(GT911_ADDR, ESP_LCD_TOUCH_GT911_READ_DATA_REG, &clear, 1);  // No touch data
  } else {
    /* Count of touched points */
    touch_cnt = buf[0] & 0x0F;
    if (touch_cnt > GT911_LCD_TOUCH_MAX_POINTS || touch_cnt == 0) {
      I2C_Write_Touch(GT911_ADDR, ESP_LCD_TOUCH_GT911_READ_DATA_REG, &clear, 1);
      return true;
    }
    /* Read all points */
    I2C_Read_Touch(GT911_ADDR, ESP_LCD_TOUCH_GT911_READ_DATA_REG+1, &buf[1], touch_cnt * 8);
    /* Clear all */
    I2C_Write_Touch(GT911_ADDR, ESP_LCD_TOUCH_GT911_READ_DATA_REG, &clear, 1);
    // printf(" points=%d \r\n",touch_cnt);
    noInterrupts(); 

    /* Number of touched points */
    if(touch_cnt > GT911_LCD_TOUCH_MAX_POINTS)
        touch_cnt = GT911_LCD_TOUCH_MAX_POINTS;
    touch_data.points = (uint8_t)touch_cnt;
    /* Fill all coordinates */
    for (i = 0; i < touch_cnt; i++) {
      if(Mirror_X)
        touch_data.coords[i].x = Touch_WIDTH - (uint16_t)(((uint16_t)buf[(i * 8) + 3] << 8) + buf[(i * 8) + 2]);  
      else
        touch_data.coords[i].x = (uint16_t)(((uint16_t)buf[(i * 8) + 3] << 8) + buf[(i * 8) + 2]);  
      if(Mirror_Y)             
        touch_data.coords[i].y = Touch_HEIGHT -(uint16_t)(((uint16_t)buf[(i * 8) + 5] << 8) + buf[(i * 8) + 4]);
      else             
        touch_data.coords[i].y = (uint16_t)(((uint16_t)buf[(i * 8) + 5] << 8) + buf[(i * 8) + 4]);
      touch_data.coords[i].strength = (uint16_t)(((uint16_t)buf[(i * 8) + 7] << 8) + buf[(i * 8) + 6]);
    }
    interrupts(); 
    // printf(" points=%d \r\n",touch_data.points);
  }
  return true;
}
void Touch_Loop(void){
  if(Touch_interrupts){
    Touch_interrupts = false;
    example_touchpad_read();
  }
}
uint8_t Touch_Get_XY(uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num) {

  assert(x != NULL);
  assert(y != NULL);
  assert(point_num != NULL);
  assert(max_point_num > 0);
  
  noInterrupts(); 
  /* Count of points */
  if(touch_data.points > max_point_num)
    touch_data.points = max_point_num;
  for (size_t i = 0; i < touch_data.points; i++) {
    x[i] = touch_data.coords[i].x;
    y[i] = touch_data.coords[i].y;
    if (strength) {
        strength[i] = touch_data.coords[i].strength;
    }
    touch_data.coords[i].x = 0;
    touch_data.coords[i].y = 0;
    touch_data.coords[i].strength = 0;
  }
  *point_num = touch_data.points;
  /* Invalidate */
  touch_data.points = 0;
  interrupts(); 
  return (*point_num > 0);
}
void example_touchpad_read(void){
  uint16_t touchpad_x[GT911_LCD_TOUCH_MAX_POINTS] = {0};
  uint16_t touchpad_y[GT911_LCD_TOUCH_MAX_POINTS] = {0};
  uint16_t strength[GT911_LCD_TOUCH_MAX_POINTS]   = {0};
  uint8_t touchpad_cnt = 0;
  Touch_Read_Data();
  uint8_t touchpad_pressed = Touch_Get_XY(touchpad_x, touchpad_y, strength, &touchpad_cnt, GT911_LCD_TOUCH_MAX_POINTS);
  if (touchpad_pressed && touchpad_cnt > 0) {
      // data->point.x = touchpad_x[0];
      // data->point.y = touchpad_y[0];
      // data->state = LV_INDEV_STATE_PR;
      printf("Touch : X=%u Y=%u num=%d\r\n", touchpad_x[0], touchpad_y[0],touchpad_cnt);
  } else {
      // data->state = LV_INDEV_STATE_REL;
  }
}
/*!
    @brief  handle interrupts
*/
uint8_t Touch_interrupts;
void IRAM_ATTR Touch_GT911_ISR(void) {
  Touch_interrupts = true;
}
