#pragma once
#include "esp_log.h"
#include <Arduino.h>

#define BAT_ADC_PIN   1
#define Measurement_offset 1.0

extern float BAT_analogVolts;

void BAT_Init(void);
float BAT_Get_Volts(void);
