#pragma once
#include "esp_log.h"
#include <Arduino.h>

// No battery ADC pin on this board -- VBAT only reaches the AXP2101, so the PMIC is the
// only source for both voltage and percentage.

extern float BAT_analogVolts;

void BAT_Init(void);
float BAT_Get_Volts(void);
