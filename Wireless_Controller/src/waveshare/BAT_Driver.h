#if defined(WAVESHARE_BOARD)

#include "esp_log.h"
#include <Arduino.h>

#define BAT_ADC_PIN 8
#define Measurement_offset 0.990476

extern float BAT_analogVolts;

void BAT_Init(void);
float BAT_Get_Volts(void);

#endif