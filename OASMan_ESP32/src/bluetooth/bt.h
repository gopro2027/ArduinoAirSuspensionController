// NOTE: This is the classic bluetooth version and will be depricated eventually
#ifndef bt_h
#define bt_h
#include "user_defines.h"
#include "components/wheel.h"
#include "BluetoothSerial.h"
#include "saveData.h"
#include "airSuspensionUtil.h"

extern BluetoothSerial bt;
void bt_cmd();

#endif