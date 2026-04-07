#include "BAT_Driver.h"

float BAT_analogVolts = 0;

void BAT_Init(void)
{
    analogReadResolution(12);
}

float BAT_Get_Volts(void)
{
    uint32_t total = 0;
    const int samples = 5;
    for (int i = 0; i < samples; i++) {
        total += analogReadMilliVolts(BAT_ADC_PIN);
        delay(4);
    }
    int Volts = total / samples;
    BAT_analogVolts = (float)(Volts * 2.0 / 1000.0) / Measurement_offset;
    return BAT_analogVolts;
}
