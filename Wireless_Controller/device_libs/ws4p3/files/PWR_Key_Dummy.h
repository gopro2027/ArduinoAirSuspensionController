#pragma once

// This device has no power key and no power latch. GPIO0 -- the BOOT button on every other
// controller -- is wired to RGB data line D6 here, so it is not usable as an input either
// (see USE_BOOT_BUTTON_FUNCTIONALITY in device_lib_exports.h). All of these are no-ops.

void power_key_setup(void);
bool power_key_pressed(void);
void power_latch_on(void);
void power_latch_off(void);
void power_enable_wakeup_lightsleep();
void power_disable_wakeup_lightsleep();
void power_enable_wakeup_deepsleep();
