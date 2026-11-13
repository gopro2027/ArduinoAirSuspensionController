#pragma once

#include <Arduino.h>
#include "esp_sleep.h"

// ---------- Board-specific config ----------
//
// The side PWR button is NOT on an ESP32 GPIO: it drives the AXP2101 PWRON pin, and the PMIC
// is configured for a 4 s press-off, so a long hold cuts the rails in hardware with no firmware
// involvement. Nothing mirrors its level onto a readable pin, so power_key_pressed() below can
// only see BOOT. Reading PWRON short presses would mean servicing the AXP2101 IRQ status.

// BOOT button, active LOW. The only button on an MCU pin, so also the only ext0 wake source.
#ifndef PWR_WAKE_GPIO
#define PWR_WAKE_GPIO 0
#endif

#ifndef PWR_WAKE_ACTIVE_LOW
#define PWR_WAKE_ACTIVE_LOW 1
#endif

// No power-latch GPIO on this board: the AXP2101 holds the rails up.
#ifndef PWR_LATCH_PIN
#define PWR_LATCH_PIN -1
#endif

#ifndef PWR_LATCH_ACTIVE_LEVEL
#define PWR_LATCH_ACTIVE_LEVEL HIGH
#endif

// ---------- API used by src/waveshare/PWR_Key.cpp ----------

void power_key_setup();
bool power_key_pressed();

void power_latch_on();
void power_latch_off();

void power_enable_wakeup_lightsleep();
void power_disable_wakeup_lightsleep();
void power_enable_wakeup_deepsleep();
