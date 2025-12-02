#ifndef directdownload_h
#define directdownload_h

#include "user_defines.h"
#include "Arduino.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include <Update.h>
#include <ArduinoJson.h>
#include "WiFiClientSecure.h"

#ifndef FIRMWARE_RELEASE_NAME
#ifdef PROJECT_IS_MANIFOLD
#define FIRMWARE_RELEASE_NAME "manifold_v2"
#else
#define FIRMWARE_RELEASE_NAME "controller_ws2p8"
#endif
#endif

enum UPDATE_STATUS
{
    UPDATE_STATUS_NONE,
    UPDATE_STATUS_SUCCESS,
    UPDATE_STATUS_FAIL_WIFI_CONNECTION,
    UPDATE_STATUS_FAIL_VERSION_REQUEST,
    UPDATE_STATUS_FAIL_FILE_REQUEST,
    UPDATE_STATUS_FAIL_GENERIC
};

extern void setupdateResult(byte value);

void downloadUpdate(String SSID, String PASS);
#ifdef WIFI_OTA_ENABLE
void startHotspot(String wifiName);
#endif

#endif