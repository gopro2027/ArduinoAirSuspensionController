#pragma once
#include "ST7789.h"

#define PWR_KEY_Input_PIN   6
#define PWR_Control_PIN     7
 
#define Device_Sleep_Time    10
#define Device_Restart_Time  15
#define Device_Shutdown_Time 20

void Fall_Asleep(void);
void Shutdown(void);
void Restart(void);

void PWR_Init(void);
void PWR_Loop(void);