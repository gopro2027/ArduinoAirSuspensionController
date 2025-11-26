#include "PWR_Key.h"

static uint8_t BAT_State = 0; 
static uint8_t Device_State = 0; 
static uint16_t Long_Press = 0;


void PWR_Loop(void)
{
  if(BAT_State){ 
    if(!gpio_get_level(PWR_KEY_Input_PIN)){   
      if(BAT_State == 2){         
        Long_Press ++;
        if(Long_Press >= Device_Sleep_Time){
          if(Long_Press >= Device_Sleep_Time && Long_Press < Device_Restart_Time)
            Device_State = 1;
          else if(Long_Press >= Device_Restart_Time && Long_Press < Device_Shutdown_Time)
            Device_State = 2;
          else if(Long_Press >= Device_Shutdown_Time)
            Shutdown(); 
        }
      }
    }
    else{
      if(BAT_State == 1)   
        BAT_State = 2;
      Long_Press = 0;
    }
  }
}
void Fall_Asleep(void)
{

}
void Restart(void)                              
{

}
void Shutdown(void)
{
  gpio_set_level(PWR_Control_PIN, false);
  LCD_Backlight = 0;        
}
void configure_GPIO(int pin, gpio_mode_t Mode)
{
    gpio_reset_pin(pin);                                     
    gpio_set_direction(pin, Mode);                          
    
}
void PWR_Init(void) {
  configure_GPIO(PWR_KEY_Input_PIN, GPIO_MODE_INPUT);    
  configure_GPIO(PWR_Control_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(PWR_Control_PIN, false);
  vTaskDelay(100);
  if(!gpio_get_level(PWR_KEY_Input_PIN)) {   
    BAT_State = 1;               
    gpio_set_level(PWR_Control_PIN, true);
  }
}
