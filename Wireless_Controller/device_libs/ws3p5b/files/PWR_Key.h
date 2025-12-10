#pragma once

#include <Arduino.h>
#include "esp_sleep.h"

// ---------- Waveshare ESP32-S3-Touch-LCD-3.5B Configuration ----------
// Based on official schematics

// Dummy GPIO (not used - button is on TCA9554 EXIO6)
#ifndef PWR_KEY_PIN
#define PWR_KEY_PIN 0
#endif

// AXP2101 IRQ is connected to TCA9554 EXIO5, which then goes to SYS_OUT
// The actual wakeup source is the TCA INT pin (pin 13) which connects to TP_INT
// Looking at the schematic, TP_INT is on EXIO2
// But for power management, we should use the AXP2101's PWRON functionality

// The physical power button (PWRON) goes directly to AXP2101 pin 30
// So we should rely on the AXP2101's internal power key handling
// For wakeup, we need to check if there's a GPIO connected to AXP_IRQ or PWROK

// Based on schematic: PWROK is on AXP2101 pin 29 and connects to ESP_EN
// This means the AXP2101 handles power-on directly through the EN pin
#ifndef PMU_IRQ_PIN
#define PMU_IRQ_PIN -1  // No direct IRQ to ESP32 GPIO - handled through EN pin
#endif

// TCA9554 I/O expander configuration
#ifndef TCA_ADDR
#define TCA_ADDR 0x20
#endif

#ifndef TCA_LCD_RSTBIT
#define TCA_LCD_RSTBIT 1  // EXIO1 = LCD_RST
#endif

#ifndef TCA_PWR_KEY_BIT
#define TCA_PWR_KEY_BIT 6  // EXIO6 (for status reading only)
#endif

#ifndef TCA_INT_BIT
#define TCA_INT_BIT 2  // EXIO2 = TP_INT (TCA interrupt pin)
#endif

// No hardware latch pin
#ifndef PWR_LATCH_PIN
#define PWR_LATCH_PIN -1
#endif

#ifndef PWR_LATCH_ACTIVE_LEVEL
#define PWR_LATCH_ACTIVE_LEVEL HIGH
#endif

// Power button is active-HIGH on TCA expander
#ifndef PWR_KEY_ACTIVE_LOW
#define PWR_KEY_ACTIVE_LOW 0
#endif

// ---------- API ----------

void power_key_setup();
bool power_key_pressed();

void power_latch_on();
void power_latch_off();

// Poll PMIC + power key (call from loop/task)
void power_key_loop();

// Call this AFTER all other init is complete to reset LCD
void power_key_reset_lcd();

// Debug: Test sleep/wake cycles
void power_key_test_sleep();

// Note: Sleep/wake functionality is handled by AXP2101 internally
// The PWROK pin controls ESP_EN directly
void power_enable_wakeup_lightsleep();
void power_disable_wakeup_lightsleep();
void power_enable_wakeup_deepsleep();