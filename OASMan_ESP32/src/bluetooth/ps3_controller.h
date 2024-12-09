#ifndef ps3_controller_h
#define ps3_controller_h
#include "user_defines.h"
#include "airSuspensionUtil.h"

#if ENABLE_PS3_CONTROLLER_SUPPORT

#include <Ps3Controller.h>

void ps3_controller_setup();
void ps3_controller_loop();

#endif

#endif