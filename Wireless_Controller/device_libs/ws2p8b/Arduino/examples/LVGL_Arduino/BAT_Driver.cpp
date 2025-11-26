#include "BAT_Driver.h"

float BAT_analogVolts = 0;

void BAT_Init(void)
{
  //set the resolution to 12 bits (0-4095)
  analogReadResolution(12);
}
float BAT_Get_Volts(void)
{
  int Volts = analogReadMilliVolts(BAT_ADC_PIN); // millivolts
  BAT_analogVolts = (float)(Volts * 3.0 / 1000.0) / Measurement_offset;
  // printf("BAT voltage : %.2f V\r\n", BAT_analogVolts);
  return BAT_analogVolts;
}