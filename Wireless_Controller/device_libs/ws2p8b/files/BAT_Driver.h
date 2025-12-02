#pragma once
#include "esp_log.h"
#include <Arduino.h> 

#define BAT_ADC_PIN   4
#define Measurement_offset 0.992857

extern float BAT_analogVolts;

void BAT_Init(void);
float BAT_Get_Volts(void);