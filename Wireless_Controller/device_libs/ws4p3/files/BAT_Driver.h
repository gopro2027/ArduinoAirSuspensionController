#pragma once
#include <Arduino.h>

/*
 * ESP32-S3-Touch-LCD-4.3 has a Li-ion charger and a battery header, but the cell voltage is NOT
 * wired to any ADC -- there is no divider on VBAT anywhere in ESP32-S3-Touch-LCD-4.3-Sch.pdf, and
 * the charger's CHRG/STDBY pins only drive the two on-board LEDs. The board's one ADC net ("AD",
 * GPIO6) goes straight out to the sensor header, so it cannot be repurposed either.
 *
 * So this is a stub, and the device sets DEFAULT_SHOW_BATTERY 0 so the status bar does not show a
 * meaningless reading. (The "Show Battery" toggle in settings still exists if someone wants it.)
 */

extern float BAT_analogVolts;

void BAT_Init(void);
float BAT_Get_Volts(void);
