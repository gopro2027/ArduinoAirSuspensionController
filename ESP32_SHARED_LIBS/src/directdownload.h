#ifndef directdownload_h
#define directdownload_h

#include "Arduino.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include <Update.h>
#include "user_defines.h"

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
    UPDATE_STATUS_FAIL_GENERIC,
    UPDATE_STATUS_FAIL_ALREADY_UP_TO_DATE,
    UPDATE_STATUS_FAIL_WIFI_PASSWORD,
    UPDATE_STATUS_FAIL_WIFI_NO_NETWORK,
    // Append only. Reported to clients as strings, never as the raw byte, so this is not a wire change.
    UPDATE_STATUS_FAIL_CORRUPT_DOWNLOAD,
    UPDATE_STATUS_FAIL_WEAK_CONNECTION,
    UPDATE_STATUS_FAIL_ROLLED_BACK
};

extern void setupdateResult(byte value);
extern byte getupdateResult();

void downloadUpdate(String SSID, String PASS);

// Call once at boot: turns a SUCCESS status whose update the bootloader rolled back into UPDATE_STATUS_FAIL_ROLLED_BACK.
void checkUpdateRolledBack();

// MD5 of the exact body the worker sent; absent from older worker deployments.
#define OTA_FIRMWARE_MD5_HEADER "X-Firmware-MD5"

#endif