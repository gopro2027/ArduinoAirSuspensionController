#pragma once
#include "WiFi.h"
#include <BLEDevice.h>

extern bool WIFI_Connection;
extern uint8_t WIFI_NUM;
extern uint8_t BLE_NUM;
extern bool Scan_finish;

uint8_t wifi_scan();
uint8_t ble_scan();
void Wireless_Test1();
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int wifi_scan_number();
int ble_scan_number();
void Wireless_Test2();