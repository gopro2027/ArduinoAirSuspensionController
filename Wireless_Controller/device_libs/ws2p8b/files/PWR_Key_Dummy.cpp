#include "PWR_Key_Dummy.h"
// this device does not have a power key, so all functions are empty
void power_key_setup(void){}
bool power_key_pressed(void){return false;}
void power_latch_on(void){}
void power_latch_off(void){}
void power_enable_wakeup_lightsleep(){}
void power_disable_wakeup_lightsleep(){}
void power_enable_wakeup_deepsleep(){}