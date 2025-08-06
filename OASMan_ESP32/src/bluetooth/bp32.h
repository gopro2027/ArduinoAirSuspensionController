#ifndef bp32_h
#define bp32_h

#include "sdkconfig.h"
#include <Arduino.h>
#include <Bluepad32.h>

#include <user_defines.h>
#include "airSuspensionUtil.h"

extern bool do_dance; // from tasks.cpp
void doDance();
void bp32_setup();
void bp32_loop();
void bp32_forgetDevices();
void bp32_setAllowConnections(bool allow);
#endif