#pragma once

#include <Arduino.h>
#include "esp_sleep.h"

// ---------- Board-specific config (adjust these as needed) ----------

// TCA9554 expander bit carrying the PWR button (schematic EXIO6 / SYS_OUT).
// The PWR button is not on an MCU GPIO -- see device_lib_exports.h.
#ifndef PWR_KEY_EXIO_BIT
#define PWR_KEY_EXIO_BIT 6
#endif

// PWR button active level: EXIO6 reads HIGH while the button is held
#ifndef PWR_KEY_ACTIVE_LOW
#define PWR_KEY_ACTIVE_LOW 0
#endif

// BOOT button GPIO -- the only button on an MCU pin, and therefore the only
// pin usable as an ext0 wake source out of light/deep sleep.
#ifndef PWR_WAKE_GPIO
#define PWR_WAKE_GPIO 0
#endif

#ifndef PWR_WAKE_ACTIVE_LOW
#define PWR_WAKE_ACTIVE_LOW 1
#endif

// GPIO that controls the power latch (keeps the board on)
#ifndef PWR_LATCH_PIN
#define PWR_LATCH_PIN -1 // -1 = no latch pin / no-op (this board latches via the AXP2101)
#endif

// Logic level that turns the latch "ON"
#ifndef PWR_LATCH_ACTIVE_LEVEL
#define PWR_LATCH_ACTIVE_LEVEL HIGH
#endif

// ---------- API used by src/waveshare/PWR_Key.cpp ----------

void power_key_setup();
bool power_key_pressed();

void power_latch_on();
void power_latch_off();

// Poll PMIC + power key (call from loop/task)
void power_key_loop();

void power_enable_wakeup_lightsleep();
void power_disable_wakeup_lightsleep();
void power_enable_wakeup_deepsleep();
