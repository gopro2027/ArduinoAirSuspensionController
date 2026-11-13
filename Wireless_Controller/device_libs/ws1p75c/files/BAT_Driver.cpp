#include "BAT_Driver.h"
#include "PMU_AXP2101.h"

float BAT_analogVolts = 0;

void BAT_Init(void)
{
    PMU_init();
}

float BAT_Get_Volts(void)
{
    float v = PMU_getBatteryVoltage_V();
    if (v > 0.0f)
        BAT_analogVolts = v;
    return BAT_analogVolts;
}
