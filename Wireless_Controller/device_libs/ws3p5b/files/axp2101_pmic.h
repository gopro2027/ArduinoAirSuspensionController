#pragma once
#include <Arduino.h>
#include <Wire.h>

// Tell XPowersLib our chip *before* including it
#ifndef XPOWERS_CHIP_AXP2101
  #define XPOWERS_CHIP_AXP2101
#endif
#include <XPowersLib.h>

// Default to your existing I2C pins if not defined elsewhere
#ifndef I2C_SDA
  #define I2C_SDA 8
#endif
#ifndef I2C_SCL
  #define I2C_SCL 7
#endif

namespace pmic {

void set_measure_period_ms(uint32_t ms);

// Initialize the AXP2101. irqPin = -1 if you didn't wire NIRQ.
bool begin(TwoWire *wire = &Wire, int sda = I2C_SDA, int scl = I2C_SCL, int irqPin = -1);

// Turn on/off the rails we use for display/touch
void enable_display_power(bool on);

// Optional helpers (no-ops unless you wire BL/TP to PMIC rails)
void set_backlight_rail(bool on);
void set_touch_rail(bool on);

using PekShortCb = void(*)();
using PekLongCb  = void(*)();

void set_power_key_handlers(PekShortCb on_short, PekLongCb on_long);

// Optional polling-friendly flags if you don’t want callbacks
bool consume_power_key_short();  // returns true once per event
bool consume_power_key_long();

// Telemetry
int   battery_percent();     // -1 if no battery
float battery_voltage();     // V
float vbus_voltage();        // V

// If you wired PMIC NIRQ, you can poll IRQs here
void poll();

// Escape hatch to access the raw driver if needed
XPowersPMU &handle();

} // namespace pmic
