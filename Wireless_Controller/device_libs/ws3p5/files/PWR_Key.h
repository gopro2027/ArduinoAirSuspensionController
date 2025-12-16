#pragma once

#include <Arduino.h>
#include "esp_sleep.h"

// ---------- Board-specific config (adjust these as needed) ----------

// GPIO used for the front-panel power button
#ifndef PWR_KEY_PIN
#define PWR_KEY_PIN 0    // TODO: set to real button pin for the 3.5" board
#endif

// GPIO that controls the power latch (keeps the board on)
#ifndef PWR_LATCH_PIN
#define PWR_LATCH_PIN -1 // -1 = no latch pin / no-op
#endif

// Logic level that turns the latch "ON"
#ifndef PWR_LATCH_ACTIVE_LEVEL
#define PWR_LATCH_ACTIVE_LEVEL HIGH
#endif

// Button active level (most boards are active-low)
#ifndef PWR_KEY_ACTIVE_LOW
#define PWR_KEY_ACTIVE_LOW 1
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
