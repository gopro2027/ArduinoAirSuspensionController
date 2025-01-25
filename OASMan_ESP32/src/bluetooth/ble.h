#ifndef ble_h
#define ble_h

// Tutorial: https://www.youtube.com/watch?v=s3yoZa6kzus&ab_channel=MoThunderz
// https://github.com/mo-thunderz/Esp32BlePart2/tree/main

/*
  Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
  Ported to Arduino ESP32 by Evandro Copercini
  updated by chegewara and MoThunderz
*/
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "airSuspensionUtil.h"
#include <BTOas.h>
#include "components/manifold.h"

void ble_setup();
void ble_loop();
void ble_notify();
void ble_create_characteristics(BLEService *pService);

#endif