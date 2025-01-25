#ifndef ble_h
#define ble_h

#include "Arduino.h"

// #include "BLEDevice.h"
#include "NimBLEDevice.h"
// #include "BLEScan.h"

#include <BTOas.h>
#include <user_defines.h>

bool connectCharacteristic(BLERemoteService *pRemoteService, BLERemoteCharacteristic *l_BLERemoteChar);
void ble_setup();
void ble_loop();

#endif