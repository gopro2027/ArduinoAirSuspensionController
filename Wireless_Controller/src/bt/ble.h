#ifndef ble_h
#define ble_h

#include "Arduino.h"

// #include "BLEDevice.h"
#include "NimBLEDevice.h"
// #include "BLEScan.h"

#include <stack>
#include <iostream>

#include <BTOas.h>
#include <user_defines.h>

#include "utils/util.h"

bool connectCharacteristic(BLERemoteService *pRemoteService, BLERemoteCharacteristic *l_BLERemoteChar);
void ble_setup();
void ble_loop();

#endif