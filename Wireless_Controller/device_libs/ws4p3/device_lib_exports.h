#pragma once

// requirements for screen
#include "files/Display_ST7262.h"
#include "files/Touch_GT911.h"
#include "files/LVGL_Driver.h"
#include "files/I2C_Driver.h"

// power key and battery definitions
#include "files/BAT_Driver.h"

// include blank power key implementation
#include "files/PWR_Key_Dummy.h"

// The ST7262 is a dumb RGB panel -- no MADCTL, nothing to rotate with. The UI therefore runs in
// the panel's native 800x480 LANDSCAPE, which the shared UI already handles via isLandscape()
// (the same layout the 2.8 uses when a user rotates it). Software rotation was not considered:
// it would mean rotating every dirty strip on the CPU on the largest panel in the fleet.
#define SUPPORTS_ROTATION 0

// Physical panel: 4.3" diagonal, 800x480 -> sqrt(800^2+480^2)/4.3 ~= 217 px/inch
#define DEVICE_DPI 217

// ───────────────────────── Hardware capability flags ─────────────────────────

// The BOOT button (K2) IS wired to GPIO0 here, same as every other ESP32 -- but Waveshare ALSO
// routes GPIO0 to the RGB bus as data line D6 (green bit G3). See Display_ST7262.h. So the button
// works at reset, for entering download mode, and is unusable afterwards: the LCD peripheral owns
// the pin. Leaving the runtime handler on would do two bad things -- pinMode(0, INPUT) detaches D6
// and kills part of the green channel, and digitalRead(0) then samples a live pixel-clock-rate
// signal, i.e. random highs and lows the handler would read as real presses. That handler can
// command all four corners to AIR UP, so this is not merely cosmetic.
#define USE_BOOT_BUTTON_FUNCTIONALITY 0

// KNOWN BOARD BEHAVIOUR, same root cause, nothing firmware can do about it:
// pressing the RESET button (K1) after the LCD has been running drops the board into the ROM
// download stub -- black screen, and on serial:
//     rst:0x1 (POWERON),boot:0x0 (DOWNLOAD(USB/UART0))
//     waiting for download
// boot:0x0 means GPIO0 read LOW when the ROM latched the boot strap. Verified on-device 2026-09-06:
//   - a SECOND press of RESET boots normally (in the download stub the LCD is not running, so
//     GPIO0 is free and R39's 10K pull-up wins) -- so nothing holds GPIO0 low permanently;
//   - it reproduces on a bare wall charger with no USB host, so it is not the CH343P's DTR line
//     through the Q1/Q2 auto-reset circuit;
//   - a cold power-on is always fine, because the panel has never been clocked at that point;
//   - a SOFTWARE reboot (esp_restart(), i.e. the OTA path) is unaffected -- field updates are safe.
// Strapping pins are latched by the ROM before any application code runs, so no driver change can
// influence this. Recovery is a second RESET press or a power cycle.


// Backlight enable (schematic net DISP) is CH422G EXIO2, an I2C expander pin -- on/off only, no
// PWM path. See the Backlight section of files/Display_ST7262.cpp.
#define HAS_BRIGHTNESS_ADJUSTMENT 0

// VBAT never reaches an ADC on this board. See files/BAT_Driver.h.
#define HAS_BATTERY_SENSE_READING 0
