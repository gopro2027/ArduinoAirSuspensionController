#ifndef BAT_Driver_h
#define BAT_Driver_h

#include "esp_log.h"
#include <Arduino.h>

#include "device_lib_exports.h"

extern float BAT_analogVolts;

void BAT_Init(void);
float BAT_Get_Volts(void);


#endif