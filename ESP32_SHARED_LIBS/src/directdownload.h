#ifndef directdownload_h
#define directdownload_h

#include "Arduino.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include <Update.h>

#ifdef PROJECT_IS_MANIFOLD
#ifdef BOARD_VERSION_ATLEAST_4
#define DOWNLOAD_FIRMWARE_BIN_URL "https://oasman.dev/oasman/firmware/manifold_v4/firmware_%s.bin"
#elif defined ACCESSORY_WIRE_FUNCTIONALITY
#define DOWNLOAD_FIRMWARE_BIN_URL "https://oasman.dev/oasman/firmware/manifold/firmware_%s.bin"
#else
#define DOWNLOAD_FIRMWARE_BIN_URL "https://oasman.dev/oasman/firmware/manifold_no_acc/firmware_%s.bin"
#endif
#endif

#ifdef PROJECT_IS_CONTROLLER
#ifdef WAVESHARE_BOARD
#define DOWNLOAD_FIRMWARE_BIN_URL "https://oasman.dev/oasman/firmware/ws2p8controller/firmware_%s.bin"
#else
#define DOWNLOAD_FIRMWARE_BIN_URL "https://oasman.dev/oasman/firmware/controller/firmware_%s.bin"
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

#endif