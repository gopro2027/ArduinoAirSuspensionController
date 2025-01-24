#ifndef ble_h
#define ble_h

#include "Arduino.h"

//#include "BLEDevice.h"
#include "NimBLEDevice.h"
// #include "BLEScan.h"

// included from base project, same files
#include "../../../OASMan_ESP32/src/bluetooth/BTOas.h"

bool connectCharacteristic(BLERemoteService *pRemoteService, BLERemoteCharacteristic *l_BLERemoteChar);
void ble_setup();
void ble_loop();

#endif