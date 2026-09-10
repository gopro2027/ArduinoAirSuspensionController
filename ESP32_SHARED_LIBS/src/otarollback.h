#ifndef otarollback_h
#define otarollback_h

#include <Arduino.h>

// A new OTA image stays unconfirmed until loop() has run this long; any reboot before then reverts it.
#ifndef OTA_VERIFY_CONFIRM_MS
#define OTA_VERIFY_CONFIRM_MS 5000UL
#endif

// Call every loop().
void otaVerifyLoop();

#endif
