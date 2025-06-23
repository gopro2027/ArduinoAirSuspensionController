#ifndef directdownload_h
#define directdownload_h

#include "Arduino.h"
#include "WiFi.h"
#include "SPIFFS.h"
#include "HTTPClient.h"
#include <Update.h>
#include "manifoldSaveData.h"

void downloadUpdate();

#endif