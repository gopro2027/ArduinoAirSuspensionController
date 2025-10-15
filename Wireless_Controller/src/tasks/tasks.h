#ifndef tasks_h
#define tasks_h

#include <Arduino.h>

#include "bt/ble.h"

#if defined(WAVESHARE_BOARD)
#include "waveshare/waveshare.h"
#endif

void setup_tasks();

#endif