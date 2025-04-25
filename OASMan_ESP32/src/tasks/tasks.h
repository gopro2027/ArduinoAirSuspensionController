#ifndef tasks_h
#define tasks_h

#include <Arduino.h>

#include <user_defines.h>
#include "components/screen.h"
#include "bluetooth/bt.h"
#include "ota/ota.h"
#include "bluetooth/ble.h"

#if ENABLE_PS3_CONTROLLER_SUPPORT
#include "bluetooth/ps3_controller.h"
#endif

void setup_tasks();
#ifdef parabolaLearn
void start_parabolaLearnTask();
#endif

#endif