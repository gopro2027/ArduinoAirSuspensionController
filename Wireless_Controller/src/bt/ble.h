#ifndef ble_h
#define ble_h

#include "Arduino.h"

#include "BLEDevice.h"
// #include "BLEScan.h"

bool connectCharacteristic(BLERemoteService *pRemoteService, BLERemoteCharacteristic *l_BLERemoteChar);

#endif