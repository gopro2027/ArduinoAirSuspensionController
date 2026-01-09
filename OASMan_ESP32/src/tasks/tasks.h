#ifndef tasks_h
#define tasks_h

#include <Arduino.h>

#include <user_defines.h>
#include "components/screen.h"
#include "bluetooth/ble.h"
#include "bluetooth/bp32.h"

void setup_tasks();

extern RfReceiver *getRfReceiver(); // defined in airSuspensionUtil.h

#endif