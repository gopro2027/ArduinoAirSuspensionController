#include "BAT_Driver.h"
#include "PWR_Key.h"

void waveshare_init()
{
    BAT_Init();
    PWR_Init();
}

static char voltsString[20];

void waveshare_loop()
{
    static uint32_t pwr_next = 0;

    auto const now = millis();
    // Power-key state machine tick every 100 ms
    if (now >= pwr_next)
    {
        PWR_Loop();
        pwr_next = now + 100; // 100 ms period → Device_*_Time in 0.1s units
    }

    static uint32_t t = 0;
    if (millis() - t > 2000)
    {
        t = millis();
        float volt = BAT_Get_Volts();
        log_i("Vbat=%.3f", volt);

        if (volt > 4.15)
        {
            snprintf(voltsString, sizeof(voltsString), "%.2fV (Charging)", volt);
        }
        else
        {
            // 4.14 = max voltage
            // 3.5 = roughly arbitrary minimum voltage
            float v_min = 3.5f;
            float v_max = 4.09f; // 4.14 is typically the value it drops to the moment you remove the charger but it drops off quickly to around 4.07 and stabilizes a bit there
            if (volt > v_max)
            {
                volt = v_max;
            }
            float percent = (volt - v_min) / (v_max - v_min);
            snprintf(voltsString, sizeof(voltsString), "%d%%", (int)(percent * 100.0f));
        }
    }
}

char *getBatteryVoltageString()
{
    return voltsString;
}