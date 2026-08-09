// device_lib_exports.h - ST7796 + FT6336 Configuration
#pragma once

#include <lvgl.h>

// Display configuration
// #ifndef DISPLAY_WIDTH
//   #define DISPLAY_WIDTH  320
// #endif
// #ifndef DISPLAY_HEIGHT
//   #define DISPLAY_HEIGHT 480
// #endif

#define LCD_WIDTH  DISPLAY_WIDTH
#define LCD_HEIGHT DISPLAY_HEIGHT

// I2C pins
// #ifndef I2C_SDA
//   #define I2C_SDA 8
// #endif
// #ifndef I2C_SCL
//   #define I2C_SCL 7
// #endif

// Touch header
#include "files/esp_lcd_touch_ft6336.h"

// ---------- Power key and battery definitions ----------
//
// The PWR side button is NOT on an ESP32 GPIO on this board. It drives the
// AXP2101 PWRON pin (K3), and a BSS138 level shifter mirrors that state onto
// the TCA9554 expander as EXIO6 (schematic net SYS_OUT), which reads HIGH
// while PWR is held down.
//   - Waveshare wiki FAQ: "the action can be judged by the high and low levels
//     of the EXIO6 detection buttons of the extended IO, and the high level is
//     pressed" (holding >6 s makes the AXP2101 cut power in hardware).
//   - ESP32-S3-Touch-LCD-3.5-Schematic.pdf: EXIO6 = SYS_OUT, K3 -> PWRON.
//
// The only button on an MCU pin is BOOT on GPIO0 (active LOW), so it is also
// the only usable ext0 sleep-wake source -- the expander sits on I2C and is
// unreachable while the SoC is asleep.

// TCA9554 bit for the PWR button. Schematic EXIO<n> == expander P<n>.
#ifndef PWR_KEY_EXIO_BIT
  #define PWR_KEY_EXIO_BIT 6  // was: PWR_KEY_Input_PIN -0, i.e. GPIO0 (the BOOT button)
#endif

#ifndef PWR_KEY_ACTIVE_LOW
  #define PWR_KEY_ACTIVE_LOW 0  // EXIO6 is HIGH while pressed ; was: 1
#endif

// BOOT button, active LOW. Doubles as the light/deep-sleep wake source.
#ifndef PWR_WAKE_GPIO
  #define PWR_WAKE_GPIO 0
#endif

#ifndef PWR_WAKE_ACTIVE_LOW
  #define PWR_WAKE_ACTIVE_LOW 1
#endif

// No power-latch GPIO on this board: the AXP2101 holds the rails up.
#ifndef PWR_LATCH_PIN
  #define PWR_LATCH_PIN -1  // was: PWR_Control_PIN -0, which drove GPIO0 (the BOOT pin) as an output
#endif

#ifndef PWR_LATCH_ACTIVE_LEVEL
  #define PWR_LATCH_ACTIVE_LEVEL HIGH
#endif

// No light-sleep path on this board. Nothing on an MCU pin changes when PWR is
// pressed, so there is no wake source to come back on -- a sleeping device
// could only be revived with BOOT or the 10-minute timeout. Skip sleep: a short
// press does nothing and a long press shuts down.
#ifndef DEVICE_SUPPORTS_LIGHT_SLEEP
  #define DEVICE_SUPPORTS_LIGHT_SLEEP 0
#endif

// This board can do a *real* power-off: the AXP2101 cuts the rails, and a later
// PWR press powers it back up in hardware. Shutdown() prefers this over the
// fake deep-sleep shutdown used by the latch-based boards.
#ifndef DEVICE_HAS_HARD_SHUTDOWN
  #define DEVICE_HAS_HARD_SHUTDOWN 1
#endif

#define SUPPORTS_ROTATION 1

// Physical panel: 3.5" diagonal, 320x480 -> ~165 px/inch
#define DEVICE_DPI 165

// Struct for LVGL initialization
struct touch_and_screen
{
    lv_indev_t *touch;
    lv_display_t *screen;
};

// Function exports
void Backlight_Init();
void Set_Backlight(uint8_t Light);
void I2C_Init(void);
void LCD_Init();
void LCD_SetRotation(uint8_t rotation);
touch_and_screen Lvgl_Init(void);

void BAT_Init(void);
float BAT_Get_Volts(void);

// Low-level power key / latch HAL (implemented in files/PWR_Key.cpp)
void power_key_setup(void);
bool power_key_pressed(void);

// No latch FET on this board -- both of these are no-ops. Dropping power is
// power_hard_shutdown(), which must NOT be wired into power_latch_off():
// PWR_Init() calls power_latch_off() on every boot as part of its latch probe.
void power_latch_on(void);
void power_latch_off(void);

// Cut the rails via the AXP2101. Does not return if it succeeds; the caller
// must keep its existing shutdown path as a backstop in case it does not.
void power_hard_shutdown(void);

void power_enable_wakeup_lightsleep(void);
void power_disable_wakeup_lightsleep(void);
void power_enable_wakeup_deepsleep(void);