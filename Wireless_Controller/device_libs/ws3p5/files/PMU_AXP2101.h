#pragma once

#include <Arduino.h>
#include <Wire.h>

// Legacy PMU facade. The implementation now delegates to axp2101_pmic.*
// so we only initialize and talk to the PMIC in one place.

// Initialise the PMU / charger; returns true if the chip ACKs on I2C.
// Safe to call more than once.
bool PMU_init();

// Battery helpers ---------------------------------------------------

// Battery voltage in millivolts. Returns 0 if the PMU is not initialised.
uint16_t PMU_getBatteryVoltage_mV();

// Battery voltage in volts (convenience wrapper).
float PMU_getBatteryVoltage_V();

// Battery state-of-charge in percent. Returns 0 if unsupported.
uint8_t PMU_getBatteryPercent();

// Optional debug / maintenance hooks (stubs / no-ops).
void PMU_loop();
void printPMU();
void enterPmuSleep(void);
