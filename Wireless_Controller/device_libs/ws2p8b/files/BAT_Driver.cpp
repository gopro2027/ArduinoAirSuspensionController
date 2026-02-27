
#include "BAT_Driver.h"

float BAT_analogVolts = 0;

void BAT_Init(void)
{
  // set the resolution to 12 bits (0-4095)
  analogReadResolution(12);
  // Not sure why exactly, but this is needed to be here or the analogReadMilliVolts below will crash.
  // Possibly something related to lazy loading, and only calling it in the function below makes it get called too late in the runtime that it freaks out? It doesn't appear to be an issue that it's lazy loaded from the task though like AI suggested the root cause may be. Anyways, it works now.
  (void)analogReadMilliVolts(BAT_ADC_PIN);
}
// 4.10 = max voltage
// 4.22 = charging voltage
float BAT_Get_Volts(void)
{
  uint32_t total = 0;
  const int samples = 5;
  for (int i = 0; i < samples; i++)
  {
    total += analogReadMilliVolts(BAT_ADC_PIN);
    delay(4);
  }
  int Volts = total / samples; // millivolts
  BAT_analogVolts = (float)(Volts * 3.0 / 1000.0) / Measurement_offset;
  //  printf("BAT voltage : %.2f V\r\n", BAT_analogVolts);
  return BAT_analogVolts;
  // return Volts;
}
