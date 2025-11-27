#ifndef PWR_Key_h
#define PWR_Key_h

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <stdint.h>

#include "bt/ble.h"

#include "board_driver_util.h"

#define PWR_KEY_ACTIVE_LOW 1    // button pulls low when pressed
#define PWR_LATCH_ACTIVE_HIGH 1 // drive HIGH to hold power

// Long-press thresholds are in loop "ticks" (calls to PWR_Loop)
#define Device_Shutdown_Time 10

void PWR_Init(void);
void PWR_Loop(void);

// Optional lifecycle hooks (override in your app if you want custom behavior)
void Fall_Asleep(void);
void Shutdown(void);


#endif