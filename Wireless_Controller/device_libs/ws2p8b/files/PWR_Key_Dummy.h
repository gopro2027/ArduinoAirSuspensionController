#pragma once

void power_key_setup(void);
bool power_key_pressed(void);
void power_latch_on(void);
void power_latch_off(void);
void power_enable_wakeup_lightsleep();
void power_disable_wakeup_lightsleep();
void power_enable_wakeup_deepsleep();