#pragma once
#include "Arduino.h"
#include "Display_ST7789.h"

#define PWR_KEY_Input_PIN   6
#define PWR_Control_PIN     7

#define Measurement_offset 0.990476     
#define EXAMPLE_BAT_TICK_PERIOD_MS 50

#define Device_Sleep_Time    10
#define Device_Restart_Time  15
#define Device_Shutdown_Time 20

void Fall_Asleep(void);
void Shutdown(void);
void Restart(void);

void PWR_Init(void);
void PWR_Loop(void);